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

	//テストノード用の入力ピンを設定
	GraphPin test_input;	//テストノード用入力ピン
	test_input.id = 2;
	test_input.name = u8"入力";
	test_input.kind = PinKind::Input;
	test_input.node_id = test_node_id;
	test_node.inputs.push_back(test_input);

	//テストノード用の出力ピンを設定
	GraphPin test_output;	//テストノード用出力ピン
	test_output.id = 3;
	test_output.name = u8"出力";
	test_output.kind = PinKind::Output;
	test_output.node_id = test_node_id;
	test_node.outputs.push_back(test_output);

	root_graph.nodes.push_back(test_node);
	layer_datas.push_back(root_graph);

	next_id = 4;
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
		ImGui::Spacing();

		//入力ピンのグループ配置
		ImGui::BeginGroup();

		//ノードが持っている全ての入力ピンを走査
		for (size_t in_idx = 0; in_idx < node.inputs.size(); in_idx++)
		{
			const GraphPin& pin = node.inputs[in_idx];	//入力ピン情報
			ed::BeginPin(pin.id, ed::PinKind::Input);
			ImGui::Text("->%s", pin.name.c_str());
			ed::EndPin();
		}

		ImGui::EndGroup();	//入力ピンのグループ終了
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(40.0f, 0.0f));
		ImGui::SameLine();

		//出力ピンのグループ配置
		ImGui::BeginGroup();

		//ノードが持っているすべての出力ピンを走査
		for (size_t out_idx = 0; out_idx < node.outputs.size(); out_idx++)
		{
			const GraphPin& pin = node.outputs[out_idx];	//出力ピンの情報
			ed::BeginPin(pin.id, ed::PinKind::Output);
			ImGui::Text("%s ->", pin.name.c_str());
			ed::EndPin();
		}
		ImGui::EndGroup();	//出力ピングループ終了

		ed::EndNode();
	}

	//グラフに存在する全ての接続線を描画
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		const GraphLink& link = current_graph->links[i];		//接続データ
		ed::Link(link.id, link.start_pin_id, link.end_pin_id);
	}

	//ピン同士のドラッグ操作による接続線の作成判定
	if (ed::BeginCreate())
	{
		CreateNewLink(current_graph);
	}
	ed::EndCreate();

	//削除操作をしたか確認
	if (ed::BeginDelete())
	{
		DeleteNode(current_graph);
		DeleteLink(current_graph);
	}
	ed::EndDelete();

	ed::Suspend();	//エディタの描画を一時停止

	static ImVec2 popup_click_pos = ImVec2(0.0f, 0.0f);	//クリック位置


	//背景がクリックされたか
	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node Context Menu");
		popup_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
	}

	if (ImGui::BeginPopup("Create New Node Context Menu"))
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
	new_node.id = next_id++;
	new_node.name = u8"新規ステート";
	new_node.position_x = click_pos.x;
	new_node.position_y = click_pos.y;
	new_node.is_sub_graph = false;
	new_node.sub_graph_id = 0;
	
	//新しい入力ピンのデータ作成
	GraphPin new_input;		//新しい入力ピン
	new_input.id = next_id++;
	new_input.name = u8"入力";
	new_input.kind = PinKind::Input;
	new_input.node_id = new_node.id;
	new_node.inputs.push_back(new_input);

	//新しい出力ピンのデータ作成
	GraphPin new_output;	//新しい出力ピン
	new_output.id = next_id++;
	new_output.name = u8"出力";
	new_output.kind = PinKind::Output;
	new_output.node_id = new_node.id;
	new_node.outputs.push_back(new_output);

	current_graph->nodes.push_back(new_node);
	ed::SetNodePosition(new_node.id, click_pos);	//ノードの初期座標を設定
	printf("StateMachineGraphEditor: ノードを追加しました。ID: %d, 入力ピンID: %d, 出力ピンID: %d\n",
		new_node.id, new_input.id, new_output.id);
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

//接続線の削除
void StateMachineGraphEditor::DeleteLink(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DeleteLink - current_graph が nullptr です。\n");
		return;
	}

	ed::LinkId delete_link_id;	//削除対象のリンクID

	//削除対象として要求されているリンクをループで取得
	while (ed::QueryDeletedLink(&delete_link_id))
	{
		//削除承認が出たか判定
		if (ed::AcceptDeletedItem())
		{
			uint32_t target_id = static_cast<uint32_t>(delete_link_id.Get());	//取得したID

			//一致するリンクを検索して走査
			for (auto it = current_graph->links.begin(); it != current_graph->links.end(); ) 
			{
				//削除対象のIDと一致するか確認
				if (it->id == target_id)
				{
					it = current_graph->links.erase(it);
					printf("StateMachineGraphEditor: リンクを削除しました。ID: %d\n", target_id);
					break;
				}
				else
				{
					it++;
				}
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

//接続線の作成を検知してデータに追加
void StateMachineGraphEditor::CreateNewLink(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::CreateNewLink - current_graph が nullptr です。\n");
		return;
	}

	ed::PinId start_pin_id;	//接続元のピンID
	ed::PinId end_pin_id;	//接続先のピンID

	//接続線の作成要求されているか確認
	if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) 
	{
		//接続を確定してか判定
		if (ed::AcceptNewItem())
		{
			GraphLink new_link;	//新しいリンク情報
			new_link.id = next_id++;
			new_link.start_pin_id = static_cast<uint32_t>(start_pin_id.Get());
			new_link.end_pin_id = static_cast<uint32_t>(end_pin_id.Get());
			current_graph->links.push_back(new_link);

			printf("StateMachineGraphEditor: リンクを作成しました。ID: %d, 出力ピン: %d -> 入力ピン: %d\n",
				new_link.id, new_link.start_pin_id, new_link.end_pin_id);
		}
	}
}

//カスタムデリータ
void StateMachineGraphEditor::EditorContexDeleter::operator()(ax::NodeEditor::EditorContext* context) const noexcept
{
	if (context)
	{
		ed::DestroyEditor(context);
	}
}
