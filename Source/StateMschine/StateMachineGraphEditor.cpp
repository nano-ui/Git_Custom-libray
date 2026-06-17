#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateMachineGraphEditor.h"
#include "../StateMschine/StateBlackboard.h"

#include <imgui_node_editor_internal.h>
#include <cassert>

namespace ed = ax::NodeEditor;

//コンストラクタ
StateMachineGraphEditor::StateMachineGraphEditor()
{
	//ライブラリ初期化
	ed::Config config;
	config.SettingsFile = "Data/Json/NodeEditor_State.json";
	editor_context.reset(ed::CreateEditor());

	//初期グラフ生成
	const uint32_t root_id = 0;
	current_graph_id = root_id;
	GraphData root_graph;
	root_graph.id = 0;
	root_graph.name = "ルート";

	//テストデータ作成
	const uint32_t test_node_id = 1;
	GraphNode test_node;
	test_node.id = test_node_id;
	test_node.name = u8"待機状態";
	test_node.position_x = 100.0f;
	test_node.position_y = 100.0f;
	test_node.is_sub_graph = false;
	test_node.sub_graph_id = 0;

	root_graph.nodes.push_back(test_node);
	layer_datas.push_back(root_graph);

	next_node_id = 2;
}

//エディタ描画
void StateMachineGraphEditor::DrawEditor(StateBlackboard* blackboard)
{
	//アクティブグラフ検索
	GraphData* current_graph = nullptr;	//現在の階層情報

	//全ての階層データから現在のグラフIDに一致するものを探す
	for (size_t i = 0; i < layer_datas.size(); i++)
	{
		if (layer_datas[i].id == current_graph_id)
		{
			current_graph = &layer_datas[i];
			break;
		}
	}

	//ループの外側で未発見チェック
	if (!current_graph)
	{
		assert(false && "StateMachineGraphEditor: 指定されたグラフIDが見つかりません。");
		return;
	}

	//画面描画
	ImGui::Begin(u8"ステートマシンエディタ");
	ed::SetCurrentEditor(editor_context.get());
	ed::Begin("Node Canvas");

	//階層内のすべてのノードを描画
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i];
		ed::BeginNode(node.id);
		ImGui::Text("%s", node.name.c_str());
		ed::EndNode();
	}

	//削除操作をしたか確認
	if (ed::BeginDelete())
	{
		DeleteNode(current_graph);
	}
	ed::EndDelete();

	ed::Suspend();	//エディタの描画を一時停止

	static ImVec2 popup_click_pos = ImVec2(0.0f, 0.0f);	//クリック位置


	//背景がクリックされたか
	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Nodo Context Menu");
		popup_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
	}

	if (ImGui::BeginPopup("Create New Nodo Context Menu"))
	{
		//項目がクリックされたか判定
		if (ImGui::MenuItem(u8"ステート追加"))
		{
			AddNode(current_graph, popup_click_pos);
		}
		ImGui::EndPopup();
	}

	ed::Resume();
	ed::End();

	DrawPropertyWindow(current_graph);

	ed::SetCurrentEditor(nullptr);
	ImGui::End();
}

//ノードの生成
void StateMachineGraphEditor::AddNode(GraphData* current_graph, const ImVec2& click_pos)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::AddNewNode - current_graph が nullptr です。\n");
		return;
	}

	GraphNode new_node;	//新しいノード情報

	//ノードのパラメータ設定
	new_node.id = next_node_id;
	new_node.name = u8"新規ステート";
	new_node.position_x = click_pos.x;
	new_node.position_y = click_pos.y;
	new_node.is_sub_graph = false;
	new_node.sub_graph_id = 0;
	current_graph->nodes.push_back(new_node);
	ed::SetNodePosition(new_node.id, click_pos);	//ノードの初期座標を設定
	printf("StateMachineGraphEditor:ノードを追加しました。ID：%d,名前：%s\n", new_node.id, new_node.name.c_str());
	next_node_id++;

}

//ノードの削除
void StateMachineGraphEditor::DeleteNode(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DeleteSelectedObjects - current_graph が nullptr です。\n");
		return;
	}

	ed::NodeId delete_node_id;	//削除対象のID格納先

	//削除要求されているノードをループで取得
	while (ed::QueryDeletedNode(&delete_node_id))
	{
		//削除してよいかの最終確認済みか判定
		if (ed::AcceptDeletedItem())
		{
			uint32_t target_id = static_cast<uint32_t>(delete_node_id.Get());	//削除対象のID
			bool is_removed = false;	//一致するノードを階層データから検索して削除するフラグ

			//現在のグラフ内の全ノードを走査
			for (auto it = current_graph->nodes.begin(); it != current_graph->nodes.end();)
			{
				//削除対象のIDと一致するか確認
				if (it->id == target_id)
				{
					std::string debug_name = it->name;	//デバッグ出力用の名前
					it = current_graph->nodes.erase(it);
					printf("StateMachineGraphEditor: ノードを削除しました。ID: %d, 名前: %s\n", target_id, debug_name.c_str());
					is_removed = true;
					break;
				}
				else
				{
					it++;
				}
			}

			//エディタ側で消したのにデータリストに見つからなかった場合
			if (!is_removed)
			{
				printf("Warning: 削除要求されたノードID: %d が layer_datas 内に見つかりませんでした。\n", target_id);
			}
		}
	}

}

//ノードの詳細情報を描画
void StateMachineGraphEditor::DrawPropertyWindow(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DrawPropertyWindow - current_graph が nullptr です。\n");
		return;
	}

	const int max_count = 1;	//取得要求の最大ノード数
	ed::NodeId selected_nodes[max_count];	//ノードのID格納コンテナ
	int select_count = ed::GetSelectedNodes(selected_nodes, max_count);	//取得数

	//選択されているノードがあるか確認
	if (select_count <= 0)
	{
		return;
	}

	uint32_t selected_id = static_cast<uint32_t>(selected_nodes[0].Get());	//取得したノードID
	GraphNode* target_node = nullptr;	//選択されたIDのノードデータコンテナ

	//現在のグラフ内の全ノードを走査
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		//ノードIDが選択されたIDを検索
		if (current_graph->nodes[i].id == selected_id)
		{
			target_node = &current_graph->nodes[i];
			break;
		}
	}

	//一致するノードが見つからなかったか判定
	if (!target_node)
	{
		printf("Warning: 選択されたノードID: %d がデータ内に見つかりません。\n", selected_id);
		return;
	}

	ImGui::Begin(u8"ステートプロパティ");

	const size_t name_buffer_size = 128;	//バッファサイズ
	char name_input_buffer[name_buffer_size] = {};	//ノードの名前
	strcpy_s(name_input_buffer, name_buffer_size, target_node->name.c_str());

	if (ImGui::InputText(u8"ステート名", name_input_buffer, name_buffer_size))
	{
		target_node->name = name_input_buffer;
	}
	ImGui::End();
}

//カスタムデリータ
void StateMachineGraphEditor::EditorContexDeleter::operator()(ax::NodeEditor::EditorContext* context) const noexcept
{
	if (context)
	{
		ed::DestroyEditor(context);
	}
}
