#include "Editor\StateMachineEditor\Core\StateGraphNavigator.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>
#include <unordered_set>

namespace ed = ax::NodeEditor;

//上部のヘッダーのパンくずナビゲーションを描画し、階層遷移が発生したかを返す
bool StateGraphNavigator::DrawHeaderNavigation(StateGraphDataManager* data_manager, uint32_t& in_out_graph_id)
{
	if (!data_manager)
	{
		printf("Error: StateGraphNavigator::DrawHeaderNavigation - data_manager が nullptr です。\n");
		return false;
	}

	const ImVec2 window_pos = ImGui::GetWindowPos();
	const float title_bar_height = ImGui::GetFrameHeight();
	constexpr float button_margin_y = 35.0f;	//ボタン下の余白
	const ImVec2 bar_pos = ImVec2(window_pos.x, window_pos.y + title_bar_height + button_margin_y);
	constexpr float bar_height_size = 35.0f;
	const ImVec2 bar_size = ImVec2(ImGui::GetWindowWidth(), bar_height_size);

	//ナビゲーションの背景帯を描画
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	constexpr ImU32 background_bar_color = IM_COL32(35, 35, 35, 255);
	draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y), background_bar_color);

	constexpr ImVec4 text_white_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, text_white_color);
	constexpr float header_font_scale = 1.2f;
	ImGui::SetWindowFontScale(header_font_scale);
	constexpr float left_padding_x = 15.0f;
	constexpr float top_padding_y = 8.0f;
	ImGui::SetCursorScreenPos(ImVec2(bar_pos.x + left_padding_x, bar_pos.y + top_padding_y));

	ImGui::Text(u8"現在の階層ID : %d", in_out_graph_id);
	ImGui::SameLine();

	uint32_t target_navigate_id = in_out_graph_id;

	//ルート階層への復帰ボタン
	constexpr uint32_t root_graph_id = 0;
	if (ImGui::Selectable(u8"/ ルート", in_out_graph_id == root_graph_id, ImGuiSelectableFlags_None, ImGui::CalcTextSize(u8" / ルート")))
	{
		target_navigate_id = root_graph_id;
	}

	//ルート以外の階層にいる場合、祖先を遡ってパンくずリストを描画
	if (in_out_graph_id != root_graph_id)
	{
		ImGui::SameLine();
		const std::vector<uint32_t> breadcurbs = BuildBreadcrumbPath(data_manager, in_out_graph_id);

		for (int i = static_cast<int>(breadcurbs.size()) - 1; i >= 0; i--)
		{
			const uint32_t path_id = breadcurbs[i];
			std::string path_name = "Unknown";

			const auto& layers = data_manager->GetGraphDatas();
			for (size_t g = 0; g < layers.size(); g++)
			{
				if (layers[g].id == path_id)
				{
					path_name = layers[g].name;
					break;
				}
			}

			const std::string display_text = " / " + path_name;
			const std::string selectable_label = display_text + "##" + std::to_string(path_id);
			const ImVec2 text_size = ImGui::CalcTextSize(display_text.c_str());

			ImGui::SameLine();
			if (ImGui::Selectable(selectable_label.c_str(), path_id == in_out_graph_id, ImGuiSelectableFlags_None, text_size))
			{
				target_navigate_id = path_id;
			}
		}
	}

	const bool is_navigated = (in_out_graph_id != target_navigate_id);
	in_out_graph_id = target_navigate_id;

	ImGui::PopStyleColor();
	constexpr float default_font_scale = 1.0f;
	ImGui::SetWindowFontScale(default_font_scale);

	constexpr float header_bottom_spacer = 5.0f;
	ImGui::SetCursorPos(ImVec2(0.0f, title_bar_height + bar_size.y + header_bottom_spacer));
	ImGui::Dummy(ImVec2(0.0f, 1.0f));

	return is_navigated;
}

//サブグラフノードのダブルクリックによる階層移動を判定・更新
void StateGraphNavigator::CheckNavigateToSubGraph(const GraphData* current_graph, uint32_t& in_out_graph_id)
{
	if (!current_graph)
	{
		printf("Error: StateGraphNavigator::CheckNavigateToSubGraph - current_graph が nullptr です。\n");
		return;
	}

	const ed::NodeId double_clicked_node_id = ed::GetDoubleClickedNode();
	if (double_clicked_node_id)
	{
		const uint32_t clicked_id = static_cast<uint32_t>(double_clicked_node_id.Get());

		for (size_t i = 0; i < current_graph->nodes.size(); i++)
		{
			const GraphNode* node = current_graph->nodes[i].get();
			NodeBasicData basic_data = node->GetNodeBasicData();

			if (basic_data.id == clicked_id)
			{
				if (basic_data.is_sub_graph)
				{
					in_out_graph_id = basic_data.sub_graph_id;
					printf("StateGraphNavigator: サブグラフ「%s」(階層ID:%u) の内部へ移動しました。\n",
						basic_data.name.c_str(), in_out_graph_id);
				}
				break;
			}
		}
	}
}

//指定された階層IDからルートまでの親ノード階層リストを遡って構築
std::vector<uint32_t> StateGraphNavigator::BuildBreadcrumbPath(StateGraphDataManager* data_manager, uint32_t state_graph_id)
{
	std::vector<uint32_t> breadcrmbs;
	uint32_t trace_id = state_graph_id;
	std::unordered_set<uint32_t> visited_set;

	while (trace_id != 0)
	{
		if (visited_set.find(trace_id) != visited_set.end())
		{
			printf("Error: StateGraphNavigator::BuildBreadcrumbPath - パンくず探索で循環参照を検知しました。階層ID: %u\n", trace_id);
			break;
		}
		visited_set.insert(trace_id);
		breadcrmbs.push_back(trace_id);

		uint32_t parent_id = 0;
		bool found_parent = false;

		const auto& layers = data_manager->GetGraphDatas();
		for (size_t g = 0; g < layers.size(); g++)
		{
			for (size_t n = 0; n < layers[g].nodes.size(); n++)
			{
				if (layers[g].nodes[n] && layers[g].nodes[n]->GetNodeBasicData().sub_graph_id == trace_id)
				{
					parent_id = layers[g].id;
					found_parent = true;
					break;
				}
			}
			if (found_parent)
			{
				break;
			}
		}

		if (found_parent)
		{
			trace_id = parent_id;
		}
		else
		{
			break;
		}
	}

	return breadcrmbs;
}
