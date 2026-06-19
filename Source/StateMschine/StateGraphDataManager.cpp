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
void StateGraphDataManager::AddNode(GraphData* current_graph, float click_x, float click_y, const std::string& node_name)
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
	new_node.name = node_name;
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

//下位階層データを生成してIDを返す
uint32_t StateGraphDataManager::CreateNewSubGraph(const std::string& name)
{
	GraphData new_graph;	//新しい階層情報

	//パラメータ設定
	new_graph.id = FetchAndIncrementId();
	new_graph.name = name;
	layer_datas.push_back(new_graph);

	printf("StateGraphDataManager: 新しい下位階層データを作成しました。階層ID: %d, 名前: %s\n", new_graph.id, name.c_str());
	return new_graph.id;
}

//階層が空の場合に初期ノードを構築
void StateGraphDataManager::CheckAndInitDefaultNode(uint32_t graph_id)
{
	//初期ノードの構築判定と生成
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		//既にノードがある場合
		if (!layer_datas[g].nodes.empty())
		{
			return;
		}

		GraphNode test_node;
		test_node.id = FetchAndIncrementId();
		test_node.name = u8"待機状態";
		test_node.position_x = 100.0f;
		test_node.position_y = 100.0f;
		test_node.is_sub_graph = false;
		test_node.sub_graph_id = 0;

		// 入力ピン設定
		GraphPin test_input;
		test_input.id = FetchAndIncrementId();
		test_input.name = u8"入力";
		test_input.kind = PinKind::Input;
		test_input.node_id = test_node.id;
		test_node.inputs.push_back(test_input);

		// 出力ピン設定
		GraphPin test_output;
		test_output.id = FetchAndIncrementId();
		test_output.name = u8"出力";
		test_output.kind = PinKind::Output;
		test_output.node_id = test_node.id;
		test_node.outputs.push_back(test_output);

		layer_datas[g].nodes.push_back(test_node);
		printf("StateGraphDataManager: 階層ID %d に初期ノード(待機状態)を作成しました。\n", graph_id);
		return;
	}
}

//ピンIDを受け取り、接続ルールに準拠しているか判定
bool StateGraphDataManager::CheckCanConnect(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id)
{
	const GraphPin* start_pin = nullptr;	//接続元のピン
	const GraphPin* end_pin = nullptr;		//接続先のピン

	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		//現在の全ノードを走査して対象のピンデータを検索
		for (size_t i = 0; i < layer_datas[g].nodes.size(); i++)
		{

			const GraphNode& node = layer_datas[g].nodes[i];

			//入力ピンから検索
			for (size_t p = 0; p < node.inputs.size(); p++)
			{
				if (node.inputs[p].id == start_pin_id)	start_pin = &node.inputs[p];
				if (node.inputs[p].id == end_pin_id)	end_pin = &node.inputs[p];
			}

			//出力ピンから検索
			for (size_t p = 0; p < node.outputs.size(); p++)
			{
				if (node.outputs[p].id == start_pin_id)	start_pin = &node.outputs[p];
				if (node.outputs[p].id == end_pin_id)	end_pin = &node.outputs[p];
			}
		}
	}

	//両方のピンがデータ内に存在するか安全性のチェック
	if (!start_pin || !end_pin)
	{
		return false;
	}

	//同じノード内のピン同士の接続は禁止
	if (start_pin->node_id == end_pin->node_id)
	{
		return false;
	}

	//同じ種類のピン同士（入力から入力、出力から出力）の接続は禁止
	if (start_pin->kind == end_pin->kind)
	{
		return false;
	}

	//入力ピンからドラッグして出力ピンへ繋ぐ逆方向の操作を禁止
	if (start_pin->kind == PinKind::Input && end_pin->kind == PinKind::Output)
	{
		return false;
	}

	return true;
}

//ノード削除
void StateGraphDataManager::DeleteNode(uint32_t graph_id, uint32_t target_node_id)
{
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		for (auto it = layer_datas[g].nodes.begin(); it != layer_datas[g].nodes.end();)
		{
			if (it->id == target_node_id)
			{
				std::string debug_name = it->name;
				it = layer_datas[g].nodes.erase(it);
				printf("StateGraphDataManager: ノードを削除しました。ID: %d, 名前: %s\n", target_node_id, debug_name.c_str());
				return;
			}
			else
			{
				it++;
			}
		}
	}
}

//リンクの削除
void StateGraphDataManager::DeleteLink(uint32_t graph_id, uint32_t target_link_id)
{
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		for (auto it = layer_datas[g].links.begin(); it != layer_datas[g].links.end();)
		{
			if (it->id == target_link_id)
			{
				it = layer_datas[g].links.erase(it);
				printf("StateGraphDataManager: リンクを削除しました。ID: %d\n", target_link_id);
				return;
			}
			else
			{
				it++;
			}
		}
	}
}

//サブグラフノードの生成
void StateGraphDataManager::AddSubGrapNode(uint32_t graph_id, float click_x, float click_y)
{
	std::string sub_graph_name = u8"新規サブグラフ";	//サブグラフの名前
	uint32_t real_sub_graph_id = CreateNewSubGraph(sub_graph_name);

	GraphNode new_node;	//新しいノード情報

	//パラメータ設定
	new_node.id = FetchAndIncrementId();
	new_node.name = sub_graph_name;
	new_node.position_x = click_x;
	new_node.position_y = click_y;
	new_node.is_sub_graph = true;
	uint32_t assigned_sub_graph_id = real_sub_graph_id;
	new_node.sub_graph_id = assigned_sub_graph_id;

	//サブグラフ用の入力ピン
	GraphPin new_input;	//新しい入力ピン情報
	new_input.id = FetchAndIncrementId();
	new_input.name = u8"入力";
	new_input.kind = PinKind::Input;
	new_input.node_id = new_node.id;
	new_node.inputs.push_back(new_input);

	//サブグラフ用の出力ピン
	GraphPin new_output;	//新しい出力ピン情報
	new_output.id =FetchAndIncrementId();
	new_output.name = u8"出力";
	new_output.kind = PinKind::Output;
	new_output.node_id = new_node.id;
	new_node.outputs.push_back(new_output);

	//全ての階層情報から、指定されたグラフIDと一致するものを検索
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id == graph_id)
		{
			layer_datas[g].nodes.push_back(new_node);
			printf("StateGraphDataManager: サブグラフノードをデータに追加しました。ID: %d, 対応下位階層ID: %d\n",
				new_node.id, new_node.sub_graph_id);
			return;
		}
	}
	printf("Error: AddSubGrapNode - 指定された階層ID %d が見つかりませんでした。\n", graph_id);
}

//既存のノードをサブグラフに変換
void StateGraphDataManager::ConvertToSubGraph(uint32_t graph_id, uint32_t node_id)
{
	//現在のグラフ内の全ノードを走査して対象のノードを検索
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		//該当する階層情報内で対象ノードを検索
		for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
		{
			GraphNode& target_node = layer_datas[g].nodes[n];	//対象ノード

			//変換対象のノードIDが一致したか確認
			if (target_node.id == node_id)
			{
				//既にサブグラフ化されている場合は早期リターン
				if (target_node.is_sub_graph)
				{
					printf("StateGraphDataManager: ノード ID:%d は既にサブグラフです。\n", node_id);
					return;
				}

				std::string original_name = target_node.name;	//対象のノード名
				uint32_t new_sub_graph_id = CreateNewSubGraph(original_name);	//新しいサブグラフID
				layer_datas[g].nodes[n].is_sub_graph = true;
				layer_datas[g].nodes[n].sub_graph_id = new_sub_graph_id;
				layer_datas[g].nodes[n].name = original_name + u8"サブステート";

				printf("StateGraphDataManager: ノード「%s」(ID:%d) をサブグラフ(階層ID:%d)へ変換完了。\n",
					layer_datas[g].nodes[n].name.c_str(), node_id, new_sub_graph_id);
				return;
			}
		}
	}
}
