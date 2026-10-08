#include "Editor\StateMachineEditor\Views\StateGraphPropertyWindow.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\GraphNode.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Engine\Core\Input.h"
#include "TransitionConditionEditor.h"
#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>
#include <Windows.h>

namespace ed = ax::NodeEditor;

// コンストラクタ
StateGraphPropertyWindow::StateGraphPropertyWindow()
{
	condition_editor = std::make_unique<TransitionConditionEditor>();
	waiting_for_key_conditon = nullptr;
	selected_output_link_index = 0;
}

// デストラクタ
StateGraphPropertyWindow::~StateGraphPropertyWindow() = default;

// プロパティウィンドウ描画
bool StateGraphPropertyWindow::DrawProperty(
	StateGraphDataManager* data_manager,
	GraphData* current_graph,
	StateBlackboard* blackboard,
	const std::vector<std::string>& anim_names)
{
	if (!current_graph)
	{
		printf("Error: StateGraphPropertyWindow::DrawProperty - current_graph が nullptr です。\n");
		return false;
	}

	bool is_changed = false;
	ImGui::Text(u8"詳細設定");
	ImGui::Spacing();

	const int max_count = 1;
	ed::NodeId selected_nodes[max_count];
	int select_count = ed::GetSelectedNodes(selected_nodes, max_count);

	ed::LinkId selected_links[max_count];
	int select_link_count = ed::GetSelectedLinks(selected_links, max_count);

	// ノード選択時のプロパティ表示
	if (select_count > 0)
	{
		uint32_t selected_node_id = static_cast<uint32_t>(selected_nodes[0].Get());
		is_changed = DrawNodeProperty(data_manager, current_graph, selected_node_id, blackboard, anim_names);
	}
	// リンク選択時のプロパティ表示
	else if (select_link_count > 0)
	{
		uint32_t selected_link_id = static_cast<uint32_t>(selected_links[0].Get());
		is_changed = DrawLinkProperty(data_manager, current_graph, selected_link_id, blackboard);
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"ノードまたは接続線を選択してください");
	}

	return is_changed;
}

// ノードプロパティ描画
bool StateGraphPropertyWindow::DrawNodeProperty(
	StateGraphDataManager* data_manager,
	GraphData* current_graph,
	uint32_t node_id,
	StateBlackboard* blackboard,
	const std::vector<std::string>& anim_names)
{
	StateGraphNode* target_node = nullptr;
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		if (current_graph->nodes[i] && current_graph->nodes[i]->GetNodeBasicData().id == node_id)
		{
			target_node = dynamic_cast<StateGraphNode*>(current_graph->nodes[i].get());
			break;
		}
	}

	if (!target_node)
	{
		return false;
	}

	bool is_changed = false;
	NodeBasicData basic_data = target_node->GetNodeBasicData();

	ImGui::Text(u8"ステート設定 (ID:%d)", basic_data.id);
	ImGui::Spacing();

	const size_t name_buffer_size = 128;
	char name_input_buffer[name_buffer_size] = {};
	strcpy_s(name_input_buffer, name_buffer_size, basic_data.name.c_str());

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText(u8"##StateNameInput1", name_input_buffer, name_buffer_size))
	{
		basic_data.name = name_input_buffer;
		target_node->SetNodeBasicData(basic_data);
		is_changed = true;

		// サブグラフの場合は階層名も同期更新
		if (basic_data.is_sub_graph && data_manager)
		{
			auto& layers = data_manager->GetGraphDatas();
			for (size_t i = 0; i < layers.size(); i++)
			{
				if (layers[i].id == basic_data.sub_graph_id)
				{
					layers[i].name = basic_data.name;
					break;
				}
			}
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::BeginTabBar("NodePropertyTabBar"))
	{
		if (ImGui::BeginTabItem(u8"アクション"))
		{
			is_changed |= DeawNodeActionSettings(target_node, anim_names);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem(u8"遷移設定"))
		{
			is_changed |= DrawNodeTransitionSettings(data_manager, current_graph, target_node, blackboard);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 red_button_color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, red_button_color);
	if (ImGui::Button(u8"このノードを削除", ImVec2(-1.0f, 30.0f)))
	{
		uint32_t remove_node_id = basic_data.id;
		ed::DeleteNode(remove_node_id);
		printf("StateGraphPropertyWindow: ノード ID:%d (%s) の削除を要求しました。\n", remove_node_id, basic_data.name.c_str());
	}
	ImGui::PopStyleColor();

	return is_changed;
}

// ノードのアクション・アニメーション設定UI
bool StateGraphPropertyWindow::DeawNodeActionSettings(StateGraphNode* target_node, const std::vector<std::string>& anim_names)
{
	if (!target_node)
	{
		return false;
	}

	bool is_changed = false;
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"[アクション設定]");

	const char* action_ui_names[] = {
		u8"待機 (Idle)",
		u8"移動 (Move)",
		u8"攻撃 (Attack)",
		u8"回避 (Dodge)",
		u8"被弾 (Damage)",
		u8"死亡 (Dead)"
	};
	constexpr int total_action_count = 6;

	int current_category = static_cast<int>(target_node->GetActionCategory());
	ImGui::Text(u8"アクションカテゴリー:");
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo(u8"ActionCategoryCombo", &current_category, action_ui_names, total_action_count))
	{
		target_node->SetActionCategory(static_cast<ActionCategory>(current_category));
		is_changed = true;
	}

	ImGui::Spacing();

	AnimationData anim_data = target_node->GetAnimationData();
	ImGui::Text(u8"再生アニメーション:");
	ImGui::SetNextItemWidth(-1.0f);

	if (!anim_names.empty())
	{
		if (ImGui::BeginCombo(u8"##AnimNameCombo", anim_data.animation_name.c_str()))
		{
			for (size_t i = 0; i < anim_names.size(); i++)
			{
				bool is_selected = (anim_data.animation_name == anim_names[i]);
				if (ImGui::Selectable(anim_names[i].c_str(), is_selected))
				{
					anim_data.animation_name = anim_names[i];
					target_node->SetAnimationData(anim_data);
					is_changed = true;
				}
				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	else
	{
		const size_t anim_buffer_size = 128;
		char anim_input_buffer[anim_buffer_size] = {};
		strcpy_s(anim_input_buffer, anim_buffer_size, anim_data.animation_name.c_str());
		if (ImGui::InputText(u8"##AnimNameInput", anim_input_buffer, anim_buffer_size))
		{
			anim_data.animation_name = anim_input_buffer;
			target_node->SetAnimationData(anim_data);
			is_changed = true;
		}
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), u8"モデルアニメーションが未ロードです");
	}

	ImGui::Spacing();
	if (ImGui::Checkbox(u8"ループ再生", &anim_data.is_loop))
	{
		target_node->SetAnimationData(anim_data);
		is_changed = true;
	}

	ImGui::Spacing();
	if (ImGui::Checkbox(u8"ルートモーション適用", &anim_data.is_root_motion))
	{
		target_node->SetAnimationData(anim_data);
		is_changed = true;

		char debug_message_buffer[256];
		sprintf_s(debug_message_buffer, sizeof(debug_message_buffer),
			"[StateGraphPropertyWindow] Node ID: %d, RootMotion changed to: %s\n",
			target_node->GetNodeBasicData().id, anim_data.is_root_motion ? "ON" : "OFF");
		OutputDebugStringA(debug_message_buffer);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	DirectX::XMFLOAT3 color = target_node->GetLinkColor();
	float imgui_color_buffer[3] = { color.x, color.y, color.z };
	ImGui::Text(u8"接続線の色:");
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::ColorEdit3(u8"##NodeLinkColorPicker", imgui_color_buffer))
	{
		target_node->SetLinkColor({ imgui_color_buffer[0], imgui_color_buffer[1], imgui_color_buffer[2] });
		is_changed = true;
	}

	return is_changed;
}

// ノードからの遷移リンク設定UI
bool StateGraphPropertyWindow::DrawNodeTransitionSettings(
	StateGraphDataManager* data_manager,
	GraphData* current_graph,
	StateGraphNode* target_node,
	StateBlackboard* blackboard)
{
	if (!target_node)
	{
		return false;
	}

	bool is_changed = false;
	const uint32_t current_node_id = target_node->GetNodeBasicData().id;

	static uint32_t last_node_id = 0;
	if (last_node_id != current_node_id)
	{
		selected_output_link_index = 0;
	}
	last_node_id = current_node_id;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.9f, 1.0f), u8"出力リンク一覧");

	if (data_manager)
	{
		std::vector<GraphLink*> departure_links = data_manager->GetLinkesFromNode(current_graph->id, current_node_id);

		if (!departure_links.empty())
		{
			if (selected_output_link_index >= static_cast<int>(departure_links.size()))
			{
				selected_output_link_index = 0;
			}
			if (selected_output_link_index < 0)
			{
				selected_output_link_index = 0;
			}

			constexpr float list_box_height_size = 80.0f;
			if (ImGui::BeginListBox(u8"##DepartureLinksList", ImVec2(-1.0f, list_box_height_size)))
			{
				for (int link_idx = 0; link_idx < static_cast<int>(departure_links.size()); link_idx++)
				{
					bool is_link_selected = (selected_output_link_index == link_idx);
					uint32_t dest_node_id = data_manager->GetNodeIdFromPinId(current_graph->id, departure_links[link_idx]->end_pin_id);
					std::string dest_node_name = u8"不明";

					for (size_t node_idx = 0; node_idx < current_graph->nodes.size(); node_idx++)
					{
						if (current_graph->nodes[node_idx] && current_graph->nodes[node_idx]->GetNodeBasicData().id == dest_node_id)
						{
							dest_node_name = current_graph->nodes[node_idx]->GetNodeBasicData().name;
							break;
						}
					}

					char item_label_buffer[128];
					sprintf_s(item_label_buffer, sizeof(item_label_buffer), u8"[%d] -> %s (LinkID:%d)",
						link_idx, dest_node_name.c_str(), departure_links[link_idx]->id);

					if (ImGui::Selectable(item_label_buffer, is_link_selected))
					{
						selected_output_link_index = link_idx;
					}
				}
				ImGui::EndListBox();
			}

			ImGui::Spacing();
			ImGui::Text(u8"選択中リンクの遷移条件:");
			if (condition_editor)
			{
				is_changed |= condition_editor->DrawConditonSettings(
					data_manager, blackboard, current_graph->id, departure_links[selected_output_link_index]);
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"このノードからの出力リンクはありません。");
			selected_output_link_index = -1;
		}
	}

	return is_changed;
}

// リンクプロパティ描画
bool StateGraphPropertyWindow::DrawLinkProperty(
	StateGraphDataManager* data_manager,
	GraphData* current_graph,
	uint32_t link_id,
	StateBlackboard* blackboard)
{
	GraphLink* target_link = nullptr;
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		if (current_graph->links[i].id == link_id)
		{
			target_link = &current_graph->links[i];
			break;
		}
	}

	if (!target_link)
	{
		printf("Warning: リンクID: %d が見つかりませんでした。\n", link_id);
		return false;
	}

	bool is_changed = false;
	ImGui::Text(u8"接続線設定 (ID:%d)", target_link->id);
	ImGui::Spacing();

	if (condition_editor && data_manager)
	{
		is_changed |= condition_editor->DrawConditonSettings(data_manager, blackboard, current_graph->id, target_link);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 red_button_color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, red_button_color);
	if (ImGui::Button(u8"この接続線を削除", ImVec2(-1.0f, 30.0f)))
	{
		uint32_t remove_link_id = target_link->id;
		ed::DeleteLink(remove_link_id);
		printf("StateGraphPropertyWindow: リンク ID:%d の削除を要求しました。\n", remove_link_id);
		is_changed = true;
	}
	ImGui::PopStyleColor();

	return is_changed;
}