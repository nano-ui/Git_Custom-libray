#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateMachineGraphEditor.h"
#include "../StateMschine/StateBlackboard.h"

#include <imgui.h>
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
	test_node.name = "待機状態";
	test_node.position_x = 100.0f;
	test_node.position_y = 100.0f;
	test_node.is_sub_graph = false;
	test_node.sub_graph_id = 0;

	root_graph.nodes.push_back(test_node);
	layer_datas.push_back(root_graph);
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
	ImGui::Begin("ステートマシンエディタ");
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
	ed::End();
	ed::SetCurrentEditor(nullptr);
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
