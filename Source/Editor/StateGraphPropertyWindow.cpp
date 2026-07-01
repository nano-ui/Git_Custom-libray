#include "StateGraphPropertyWindow.h"
#include "../Gameplay/StateMachine/StateGraphDataManager.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"
#include "../Engine/Core/Input.h"
#include "TransitionConditionEditor.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//コンストラクタ
StateGraphPropertyWindow::StateGraphPropertyWindow()
{
	condition_editor = std::make_unique<TransitionConditionEditor>();
	waiting_for_key_conditon = nullptr;
	selected_output_link_index = 0;
}

//デストラクタ
StateGraphPropertyWindow::~StateGraphPropertyWindow() = default;

//プロパティウィンドウの全体描画
bool StateGraphPropertyWindow::DrawProperty(
	StateGraphDataManager* data_manager,
	GraphData* current_graph, 
	StateBlackboard* blackboard,
	const std::vector<std::string>& anim_names)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateGraphPropertyWindow::DrawProperty - current_graph が nullptr です。\n");
		return false;
	}

	bool is_changed = false;	//変更検知フラグ

	ImGui::Text(u8"【ステートプロパティ】");
	ImGui::Spacing();

	const int max_count = 1;	//取得要求の最大ノード数
	ed::NodeId selected_nodes[max_count];	//ノードのID格納コンテナ
	int select_count = ed::GetSelectedNodes(selected_nodes, max_count);	//取得数

	ed::LinkId selected_links[max_count];	//選択中のリンクID配列
	int select_link_count = ed::GetSelectedLinks(selected_links, max_count); // 取得数

	//選択されているノードがあるか確認
	if (select_count > 0)
	{
		uint32_t selected_node_id = static_cast<uint32_t>(selected_nodes[0].Get()); //キャストID
		is_changed = DrawNodeProperty(data_manager, current_graph, selected_node_id, blackboard, anim_names);
	}
	else if (select_link_count > 0)
	{
		uint32_t selected_link_id = static_cast<uint32_t>(selected_links[0].Get()); //キャストID
		is_changed = DrawLinkProperty(data_manager, current_graph, selected_link_id, blackboard);
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"キャンバス上の要素を\n選択すると詳細が表示されます");
	}

	return is_changed;
}

//ノード選択時の詳細プロパティ描画
bool StateGraphPropertyWindow::DrawNodeProperty(
	StateGraphDataManager* data_manager,
	GraphData* current_graph,
	uint32_t node_id,
	StateBlackboard* blackboard,
	const std::vector<std::string>& anim_names)
{
	GraphNode* target_node = nullptr;	//編集対象ノード

	//階層データ内から該当ノードを検索
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		if (current_graph->nodes[i].id == node_id)
		{
			target_node = &current_graph->nodes[i];
			break;
		}
	}

	if (!target_node)
	{
		printf("Warning: 選択されたノードID: %d がデータ内に見つかりません。\n", node_id);
		return false;
	}

	bool is_changed = false;	//値変更フラグ

	ImGui::Text(u8"【ステート設定(ID：%d)】", target_node->id);
	ImGui::Spacing();

	const size_t name_buffer_size = 128;	//バッファサイズ
	char name_input_buffer[name_buffer_size] = {};	//入力バッファ

	strcpy_s(name_input_buffer, name_buffer_size, target_node->name.c_str());

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText(u8"##StateNameInput1", name_input_buffer, name_buffer_size))
	{
		target_node->name = name_input_buffer;
		is_changed = true;

		//サブグラフ名との同期
		if (target_node->is_sub_graph && data_manager)
		{
			for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
			{
				if (data_manager->GetLayerDatas()[i].id == target_node->sub_graph_id)
				{
					data_manager->GetLayerDatas()[i].name = target_node->name;
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
		if (ImGui::BeginTabItem(u8"アクション・アニメーション"))
		{
			is_changed |= DeawNodeActionSettings(target_node, anim_names);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem(u8"遷移条件"))
		{
			is_changed |= DrawNodeTransitionSettings(data_manager, current_graph, target_node, blackboard);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}


	//削除ボタン
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 red_button_color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f); // 削除ボタン用の赤色
	ImGui::PushStyleColor(ImGuiCol_Button, red_button_color);

	//プロパティウィンドウ内に配置する削除実行ボタン
	if (ImGui::Button(u8"ステートを削除する", ImVec2(-1.0f, 30.0f)))
	{
		uint32_t remove_node_id = target_node->id;	//削除対象の確定ID
		ed::DeleteNode(remove_node_id);
		printf("StateGraphPropertyWindow: プロパティ画面からノード ID:%d (%s) の削除要求を発行しました。\n",
			remove_node_id, target_node->name.c_str());
	}
	ImGui::PopStyleColor();

	return is_changed;
}

//ノードのアクションとアニメーション設定に関するUI描画
bool StateGraphPropertyWindow::DeawNodeActionSettings(GraphNode* target_node, const std::vector<std::string>& anim_names)
{
	bool is_changed = false;	//値変更フラグ

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"[アクションとアニメーション]");

	const char* action_ui_names[] = { u8"待機",u8"移動",u8"空中",u8"攻撃",u8"回避",u8"固有技" };	//UIリスト
	const int total_action_count = 6;	//アクション総数

	ImGui::Text(u8"アクション");
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo(u8"ActionCategoryCombo", &target_node->action_category, action_ui_names, total_action_count))
	{
		is_changed = true;
	}

	ImGui::Spacing();

	ImGui::Text(u8"再生アニメーション");
	ImGui::SetNextItemWidth(-1.0f);

	if (!anim_names.empty())
	{
		// モデルから直接抽出されたアニメーション名をドロップダウンリストで描画
		if (ImGui::BeginCombo(u8"##AnimNameCombo", target_node->animation_name.c_str()))
		{
			// リストに含まれる全アニメーション名をループ走査
			for (size_t i = 0; i < anim_names.size(); i++)
			{
				bool is_selected = (target_node->animation_name == anim_names[i]);	// 現在選択されている項目か判定

				// ドロップダウン内の項目が選択されたか判定
				if (ImGui::Selectable(anim_names[i].c_str(), is_selected))
				{
					target_node->animation_name = anim_names[i];
					is_changed = true;
				}

				// 現在選択中の項目に初期フォーカスを合わせるか判定
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
		strcpy_s(anim_input_buffer, anim_buffer_size, target_node->animation_name.c_str());

		if (ImGui::InputText(u8"##AnimNameInput", anim_input_buffer, anim_buffer_size))
		{
			target_node->animation_name = anim_input_buffer;
			is_changed = true;
		}
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), u8"※モデルが未選択です。上部メニューからモデルを選択してください");
	}

	ImGui::Spacing();

	if (ImGui::Checkbox(u8"アニメーションをループ再生する", &target_node->is_loop))
	{
		is_changed = true;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	constexpr size_t color_element_count = 3;	//カラーバッファの要素数
	float imgui_color_buffer[color_element_count] = { target_node->link_color_r, target_node->link_color_g, target_node->link_color_b };	//色編集用のバッファ配列

	ImGui::Text(u8"出発リンクの色設定");
	ImGui::SetNextItemWidth(-1.0f);

	//カラーピッカーで色が変更されたかを判定する条件
	if (ImGui::ColorEdit3(u8"##NodeLinkColorPicker", imgui_color_buffer))	
	{
		target_node->link_color_r = imgui_color_buffer[0];
		target_node->link_color_g = imgui_color_buffer[1];
		target_node->link_color_b = imgui_color_buffer[2];
		is_changed = true;
	}

	return is_changed;
}

//ノードから出発する遷移線とその条件に関するUI描画
bool StateGraphPropertyWindow::DrawNodeTransitionSettings(StateGraphDataManager* data_manager, GraphData* current_graph, GraphNode* target_node, StateBlackboard* blackboard)
{
	bool is_changed = false;	//値変更フラグ

	static uint32_t last_node_id = 0;	//前回処理したノードのID

	//ノードの新規選択切り替えを検知
	if (last_node_id != target_node->id)
	{
		selected_output_link_index = 0;
		last_node_id = target_node->id;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.9f, 1.0f), u8"【出発する遷移線の条件設定】");

	//データマネージャーの有効性を確認
	if (data_manager)
	{
		std::vector<GraphLink*> departure_links = data_manager->GetLinkesFromNode(current_graph->id, target_node->id); //出発元のリンクポインタを格納

		//リンクが1つ以上存在するかを確認
		if (!departure_links.empty())
		{
			//インデックスの最大範囲外をチェック
			if (selected_output_link_index >= static_cast<int>(departure_links.size()))
			{
				selected_output_link_index = 0;
			}
			//インデックスが負の範囲外かをチェック
			if (selected_output_link_index < 0)
			{
				selected_output_link_index = 0;
			}

			constexpr float list_box_height_size = 80.0f;	//リストボックスの縦幅

			//リストボックスの描画スコープが有効かをチェック
			if (ImGui::BeginListBox(u8"##DepartureLinksList", ImVec2(-1.0f, list_box_height_size)))
			{
				//取得したリンクの数だけループ
				for (int link_idx = 0; link_idx < static_cast<int>(departure_links.size()); link_idx++)
				{
					bool is_link_selected = (selected_output_link_index == link_idx);	//在選択中の項目かどうかの判定フラグ
					uint32_t dest_node_id = data_manager->GetNodeIdFromPinId(current_graph->id, departure_links[link_idx]->end_pin_id);	//遷移先のノードIDを逆引き
					std::string dest_node_name = u8"不明なステート";	//遷移先のステート名

					//階層内の全ノードを走査
					for (size_t node_idx = 0; node_idx < current_graph->nodes.size(); node_idx++)
					{
						//目的の遷移先IDと一致したかを判定
						if (current_graph->nodes[node_idx].id == dest_node_id)
						{
							dest_node_name = current_graph->nodes[node_idx].name;
							break;
						}
					}
					constexpr size_t text_buffer_capacity = 128;	//文字バッファの容量
					char item_label_buffer[text_buffer_capacity];	//表示文字を格納
					sprintf_s(item_label_buffer, sizeof(item_label_buffer), u8"遷移線 [%d] -> %s (ID:%d)", link_idx, dest_node_name.c_str(), departure_links[link_idx]->id);

					//項目がクリックされたかを判定
					if (ImGui::Selectable(item_label_buffer, is_link_selected))
					{
						selected_output_link_index = link_idx;
					}
				}
				ImGui::EndListBox();
			}
			ImGui::Spacing();
			ImGui::Text(u8"選択中の遷移線の条件編集:");

			//条件エディターインスタンスの有効性を確認
			if (condition_editor)
			{
				is_changed |= condition_editor->DrawConditonSettings(data_manager, blackboard, current_graph->id, departure_links[selected_output_link_index]);
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"このステートから出発する遷移線はありません。\nキャンバス上で出力ピンから次のノードへ線を引いてください。");
			selected_output_link_index = -1;
		}
	}
	return is_changed;
}

//リンク選択時の詳細プロパティ
bool StateGraphPropertyWindow::DrawLinkProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t link_id, StateBlackboard* blackboard)
{
	GraphLink* target_link = nullptr;	//編集対象リンク

	//階層データ内から該当リンクを検索
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
		printf("Warning: 選択されたリンクID: %d がデータ内に見つかりません。\n", link_id);
		return false;
	}
	bool is_changed = false;	//変更検知フラグ

	//遷移条件プロパティのUI描画
	ImGui::Text(u8"遷移線設定(ID：%d)", target_link->id);
	ImGui::Spacing();

	if (condition_editor && data_manager)
	{
		is_changed |= condition_editor->DrawConditonSettings(data_manager, blackboard, current_graph->id, target_link);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 red_button_color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f); // 削除ボタン用の赤色
	ImGui::PushStyleColor(ImGuiCol_Button, red_button_color);

	//プロパティウィンドウ内に配置するリンクの削除実行ボタン
	if (ImGui::Button(u8"遷移線を削除する", ImVec2(-1.0f, 30.0f)))
	{
		uint32_t remove_link_id = target_link->id; // 削除対象となる確定リンクID
		ed::DeleteLink(remove_link_id);
		printf("StateGraphPropertyWindow: プロパティ画面からリンク ID:%d の削除要求を発行しました。\n", remove_link_id);
		is_changed = true;
	}
	ImGui::PopStyleColor();

	return is_changed;
}

//入力チェック条件専用のImGui入力UI描画
void StateGraphPropertyWindow::DrawInputCompareUI(GraphTransitionCondition& conditon)
{

}
