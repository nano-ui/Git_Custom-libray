#include "StateGraphPropertyWindow.h"
#include "../Gameplay/StateMachine/StateGraphDataManager.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"
#include "TransitionConditionEditor.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//コンストラクタ
StateGraphPropertyWindow::StateGraphPropertyWindow()
{
	condition_editor = std::make_unique<TransitionConditionEditor>();
}

//デストラクタ
StateGraphPropertyWindow::~StateGraphPropertyWindow() = default;

//プロパティウィンドウの全体描画
void StateGraphPropertyWindow::DrawProperty(StateGraphDataManager* data_manager, GraphData* current_graph, StateBlackboard* blackboard)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateGraphPropertyWindow::DrawProperty - current_graph が nullptr です。\n");
		return;
	}

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
		DrawNodeProperty(data_manager, current_graph, selected_node_id, blackboard);
	}
	else if (select_link_count > 0)
	{
		uint32_t selected_link_id = static_cast<uint32_t>(selected_links[0].Get()); //キャストID
		DrawLinkProperty(data_manager, current_graph, selected_link_id, blackboard);
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"キャンバス上の要素を\n選択すると詳細が表示されます");
	}
}

//ノード選択時の詳細プロパティ描画
void StateGraphPropertyWindow::DrawNodeProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t node_id, StateBlackboard* blackboard)
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
		return;
	}

	ImGui::Text(u8"【ステート設定(ID：%d)】", target_node->id);
	ImGui::Spacing();

	const size_t name_buffer_size = 128;	//バッファサイズ
	char name_input_buffer[name_buffer_size] = {};	//入力バッファ

	strcpy_s(name_input_buffer, name_buffer_size, target_node->name.c_str());

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText(u8"##StateNameInput1", name_input_buffer, name_buffer_size))
	{
		target_node->name = name_input_buffer;

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

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"[アクションとアニメーション]");

	const char* action_ui_names[] = { u8"待機",u8"移動",u8"空中",u8"攻撃",u8"回避",u8"固有技" };	//UIリスト
	const int total_action_count = 6;	//アクション総数

	ImGui::Text(u8"アクション");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::Combo(u8"ActionCategoryCombo", &target_node->action_category, action_ui_names, total_action_count);

	ImGui::Spacing();

	ImGui::Text(u8"再生アニメーション");
	ImGui::SetNextItemWidth(-1.0f);

	std::vector<std::string> anim_list;	//アニメーションリストコンテナ
	const std::string anim_list_key = "AnimationList";

	if (blackboard)
	{
		std::vector<std::string> empty_fallback = {};	//取得失敗時の空リスト
		anim_list = blackboard->GetValue<std::vector<std::string>>(anim_list_key, empty_fallback);
	}

	//取得したリストに中身が存在するか判定
	if (!anim_list.empty())
	{
		//モデルから取得したアニメーション名をドロップダウンリストで描画
		if (ImGui::BeginCombo(u8"##AnimNameCombo", target_node->animation_name.c_str()))
		{
			for (size_t i = 0; i < anim_list.size(); i++)
			{
				bool is_selected = (target_node->animation_name == anim_list[i]);	//現在選択されているかフラグ

				if (ImGui::Selectable(anim_list[i].c_str(), is_selected))
				{
					target_node->animation_name = anim_list[i];
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
		const size_t anim_buffer_size = 128;	//バッファサイズ
		char anim_input_buffer[anim_buffer_size] = {};	//入力バッファ
		strcpy_s(anim_input_buffer, anim_buffer_size, target_node->animation_name.c_str());

		if (ImGui::InputText(u8"##AnimNameInput", anim_input_buffer, anim_buffer_size))
		{
			target_node->animation_name = anim_input_buffer;
		}
		ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), u8"※ブラックボードに AnimationList がありません");
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
}

//リンク選択時の詳細プロパティ
void StateGraphPropertyWindow::DrawLinkProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t link_id, StateBlackboard* blackboard)
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
		return;
	}

	//遷移条件プロパティのUI描画
	ImGui::Text(u8"遷移線設定(ID：%d)", target_link->id);
	ImGui::Spacing();

	if (condition_editor && data_manager)
	{
		condition_editor->DrawConditonSettings(data_manager, blackboard, current_graph->id, target_link);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.2f, 1.0f), u8"※ここにブラックボード変数を用いた");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.2f, 1.0f), u8"  条件式(==, !=, >, <)の設定項目が");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.2f, 1.0f), u8"  並ぶようになります。");

	ImGui::Spacing();
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
	}
	ImGui::PopStyleColor();
}