#include "StateGraphDataManager.h"

#include <cstdio>

//コンストラクタ
StateGraphDataManager::StateGraphDataManager()
{
	next_id = 100;
	GraphData root_graph;	//ルート階層情報
	root_graph.id = 0;
	root_graph.name = u8"ルート";
	layer_datas.push_back(root_graph);
}

//ノードの生成
void StateGraphDataManager::AddNode(GraphData* current_graph, float click_x, float click_y)
{
	//渡されたポインタが安全か確認
	if (!current_graph)
	{
		printf("Error: GraphDataManager::AddNode - current_graph が nullptr です。\n");
		return;
	}

	GraphNode new_node;	//新しいノード情報

	//ノードのパラメータ設定
	new_node.id = next_id++;
	new_node.name = u8"新規ステート";
	new_node.position_x = click_x;
	new_node.position_y = click_y;
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

	printf("GraphDataManager: ノードデータを正常に追加しました。ID: %d, 入力ピンID: %d, 出力ピンID: %d\n",
		new_node.id, new_input.id, new_output.id);
}
