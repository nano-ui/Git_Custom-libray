#include "StateGraphPaletteWindow.h"
#include "../Gameplay/StateMachine/StateGraphDataManager.h"

#include <imgui.h>
#include <cstdio>

//パレットウィンドウの全体描画
void StateGraphPaletteWindow::DrawPalette(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t& out_focus_node_id)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateGraphPaletteWindow::DrawPalette - current_graph が nullptr です。\n");
		return;
	}

	ImGui::Spacing();

	//左ペインをタブで分割して、機能を行き来できるようにする
	if (ImGui::BeginTabBar("LeftSidebarTabBar"))
	{
		//現在の階層ステート一覧
		if (ImGui::BeginTabItem(u8"階層ノード"))
		{
			DrawHierarchyNodeList(current_graph, out_focus_node_id);
			ImGui::EndTabItem();
		}

		//全てのステートからポップ追加
		if (ImGui::BeginTabItem(u8"ステート追加"))
		{
			ImGui::Spacing();

			DrawPaletterFilterButtons();

			const float list_box_height = ImGui::GetContentRegionAvail().y;	//残りの縦幅
			ImGui::BeginChild("PeletteListChild", ImVec2(0.0f, list_box_height), true);

			const float button_offset_x = 65.0f;	//右端からボタンを引き算するオフセット幅

			DrawNormalStatePalette(data_manager, button_offset_x);
			DrawSubGraphPalette(data_manager, button_offset_x);

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

//階層ノードタブを描画
void StateGraphPaletteWindow::DrawHierarchyNodeList(GraphData* current_graph, uint32_t& out_focus_node_id)
{
	ImGui::Spacing();
	const float list_box_height = ImGui::GetContentRegionAvail().y;	//残りの縦幅

	ImGui::BeginChild("LeftStateListChild", ImVec2(0.0f, list_box_height), true);

	//現在の階層内の全ノードを走査してリストアップ
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i];	//ノード情報
		ImGui::Text("ID : %d[%s]", node.id, node.name.c_str());
		ImGui::SameLine(ImGui::GetWindowWidth() - 115.0f);
		std::string button_label = u8"フォーカス##" + std::to_string(node.id);	//ボタンラベル

		//ボタンがクリックされたか判定
		if (ImGui::Button(button_label.c_str()))
		{
			out_focus_node_id = node.id;
			printf("StateGraphPaletteWindow: ノード ID:%d (%s) へのフォーカスを予約しました。\n",
				node.id, node.name.c_str());
		}
	}
	ImGui::EndChild();
}

// パレットの切り替えフィルターボタン描画
void StateGraphPaletteWindow::DrawPaletterFilterButtons()
{
	PaletteFilter next_filter = current_filter;	//次フレームから適用するフィルター状態
	const ImVec4 active_color = ImVec4(0.2f, 0.6f, 0.4f, 1.0f);	//選択中の色

	//全て表示ボタンの処理
	if (current_filter == PaletteFilter::ALL)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, active_color);
	}
	if (ImGui::Button(u8"全て適用"))
	{
		next_filter = PaletteFilter::ALL;
	}
	if (current_filter == PaletteFilter::ALL)
	{
		ImGui::PopStyleColor();
	}

	ImGui::SameLine();
	
	//サブグラフのみボタンの処理
	if (current_filter == PaletteFilter::SubGraph)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, active_color);
	}
	if (ImGui::Button(u8"サブグラフのみ適用"))
	{
		next_filter = PaletteFilter::SubGraph;
	}
	if (current_filter == PaletteFilter::SubGraph)
	{
		ImGui::PopStyleColor();
	}

	ImGui::SameLine();

	//通常ボタンの処理
	if (current_filter == PaletteFilter::Normal)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, active_color);
	}
	if (ImGui::Button(u8"サブグラフ以外適用"))
	{
		next_filter = PaletteFilter::Normal;
	}
	if (current_filter == PaletteFilter::Normal)
	{
		ImGui::PopStyleColor();
	}
	current_filter = next_filter;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

//通常ステートのパレット項目描画
void StateGraphPaletteWindow::DrawNormalStatePalette(StateGraphDataManager* data_manager, float button_offset_x)
{
	//サブグラフの描画判定
	if (current_filter == PaletteFilter::ALL || current_filter == PaletteFilter::Normal)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), u8"▼ 通常ステート");
		ImGui::Separator();

		std::vector<std::string> normal_names = GetExistingNormalStateNames(data_manager); // サブステートリスト 

		for (size_t i = 0; i < normal_names.size(); i++)
		{
			ImGui::Text("・%s", normal_names[i].c_str());

			//直前に描画したTextアイテムをマウスで掴んで引っ張れるように設定
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::Text(u8"移動中：%s", normal_names[i].c_str());
				size_t payload_size = normal_names[i].size() + 1;	//ヌル終端文字を含めた送信バイトサイズ
				ImGui::SetDragDropPayload("DND_PAYLOAD_SUB", normal_names[i].c_str(), payload_size);
				ImGui::EndDragDropSource();
			}

			ImGui::SameLine(ImGui::GetWindowWidth() - button_offset_x);

			std::string add_btn_label = u8"追加##Sub" + std::to_string(i); //サブステート用固有IDラベル 

			if (ImGui::Button(add_btn_label.c_str()))
			{
				pending_add_palette_node_name = normal_names[i];
				pending_add_is_sub_graph = true;
				printf("StateGraphPaletteWindow: パレットから通常「%s」の追加を予約しました。\n", pending_add_palette_node_name.c_str());
			}
			ImGui::Separator();
		}
		ImGui::Spacing();
	}
}

//サブグラフのパレット項目を描画
void StateGraphPaletteWindow::DrawSubGraphPalette(StateGraphDataManager* data_manager, float button_offset_x)
{
	// サブグラフの描画判定
	if (current_filter == PaletteFilter::ALL || current_filter == PaletteFilter::SubGraph)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), u8"▼ サブステート"); // 色とテキストを見やすく変更
		ImGui::Separator();

		std::vector<std::string> sub_graph_names = GetExistingSubGraphNames(data_manager); // サブステートリスト 

		for (size_t i = 0; i < sub_graph_names.size(); i++)
		{
			ImGui::Text("・%s", sub_graph_names[i].c_str());

			//直前に描画したTextアイテムをマウスで掴んで引っ張れるように設定
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::Text(u8"移動中：%s", sub_graph_names[i].c_str());
				size_t payload_size = sub_graph_names[i].size() + 1;	//ヌル終端文字を含めた送信バイトサイズ
				ImGui::SetDragDropPayload("DND_PAYLOAD_SUB", sub_graph_names[i].c_str(), payload_size);
				ImGui::EndDragDropSource();
			}

			ImGui::SameLine(ImGui::GetWindowWidth() - button_offset_x);

			std::string add_btn_label = u8"追加##Sub" + std::to_string(i); // サブステート用固有IDラベル 

			if (ImGui::Button(add_btn_label.c_str()))
			{
				pending_add_palette_node_name = sub_graph_names[i];
				pending_add_is_sub_graph = true;
				printf("StateGraphPaletteWindow: パレットから「%s」サブグラフの追加を予約しました。\n", pending_add_palette_node_name.c_str());
			}
			ImGui::Separator();
		}
		ImGui::Spacing();
	}
}

//通常ノード名を全データから取得
std::vector<std::string> StateGraphPaletteWindow::GetExistingNormalStateNames(StateGraphDataManager* data_manager)
{
	std::vector<std::string> unique_names;	//通常ステート用コンテナ

	//データマネージャが有効か確認
	if (!data_manager) return unique_names;

	//全ての階層情報を巡回して通常ノード名を収集
	for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
	{
		const GraphData& graph = data_manager->GetLayerDatas()[g];	//対象の階層

		//階層内のすべてのノードを巡回
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			//通常ステートのみを抽出
			if (!graph.nodes[n].is_sub_graph)
			{
				const std::string& node_name = graph.nodes[n].name;	//ノード名
				bool is_duplicate = false;	//重複管理フラグ

				//コンテナを巡回
				for (size_t i = 0; i < unique_names.size(); i++)
				{
					//名前が一致しているか確認
					if (unique_names[i] == node_name)
					{
						is_duplicate = true;
						break;
					}
				}

				//重複していないか確認
				if (!is_duplicate)
				{
					unique_names.push_back(node_name);
				}
			}
		}
	}
	return unique_names;
}

//サブグラフ名を全データから取得
std::vector<std::string> StateGraphPaletteWindow::GetExistingSubGraphNames(StateGraphDataManager* data_manager)
{
	std::vector<std::string> unique_names;	//サブグラフ用コンテナ

	//データマネージャが有効か確認
	if (!data_manager) return unique_names;

	//全ての階層情報を巡回してサブグラフノード名を収集
	for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
	{
		const GraphData& graph = data_manager->GetLayerDatas()[g];	//対象の階層

		//階層内のすべてのノードを巡回
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			//サブグラフステートのみを抽出
			if (graph.nodes[n].is_sub_graph)
			{
				const std::string& node_name = graph.nodes[n].name;	//ノード名
				bool is_duplicate = false;	//重複管理フラグ

				//コンテナを巡回
				for (size_t i = 0; i < unique_names.size(); i++)
				{
					//名前が一致しているか確認
					if (unique_names[i] == node_name)
					{
						is_duplicate = true;
						break;
					}
				}

				//重複していないか確認
				if (!is_duplicate)
				{
					unique_names.push_back(node_name);
				}
			}
		}
	}
	return unique_names;
}