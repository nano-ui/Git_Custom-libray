#include "StateMachineGraphEditor.h"
#include "../StateMschine/StateBlackboard.h"

#include <imgui.h>
#include <cassert>

//コンストラクタ
StateMachineGraphEditor::StateMachineGraphEditor()
{
	current_graph_id = 0;
	GraphData root_graph;
	root_graph.id = 0;
	root_graph.name = "ルート";
	layer_datas.push_back(root_graph);
}

//エディタ描画
void StateMachineGraphEditor::DrawEditor(StateBlackboard* blackboard)
{
	GraphData* current_graph = nullptr;	//現在の階層情報

	for (size_t i = 0; i < layer_datas.size(); i++)
	{
		if (layer_datas[i].id == current_graph_id)
		{
			current_graph = &layer_datas[i];
			break;
		}

		if (!current_graph)
		{
			assert(false && "StateMachineGraphEditor: 指定されたグラフIDが見つかりません。");
			break;
		}
	}

	

}
