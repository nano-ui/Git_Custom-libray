#include "GraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\GraphNode.h"

#include <cstdio>

//コンストラクタ
GraphDataManager::GraphDataManager()
{
	next_id = 100;

	GraphData root_graph;
	root_graph.id = 0;
	root_graph.name = u8"ルート";
	layer_datas.push_back(std::move(root_graph));
}

//ID発行処理
uint32_t GraphDataManager::FetchAndIncrementId()
{
	uint32_t current_id = next_id;
	next_id++;
	return current_id;
}

//リンクの削除
void GraphDataManager::DeleteLink(uint32_t graph_id, uint32_t target_link_id)
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
				printf("GraphDataManager: リンクを削除しました。ID: %d\n", target_link_id);
				return;
			}
			else
			{
				it++;
			}
		}
	}
}

//指定されたノードIDが所属する階層のIDを検索して取得
uint32_t GraphDataManager::GetGraphIdFromNodeId(uint32_t node_id)
{
	uint32_t target_graph_id = UINT32_MAX;	//検索結果のグラフID

	//全ての階層データを巡回
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		//階層内の全ノードを走査
		for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
		{
			//目的のノードIDと一致したかを判定
			if (layer_datas[g].nodes[n]->GetNodeBasicData().id == node_id)
			{
				target_graph_id = layer_datas[g].id;
				return target_graph_id;
			}
		}
	}

	if (target_graph_id == UINT32_MAX)
	{
		printf("Warning: GraphDataManager::GetGraphIdFromNodeId - ノードID:%d が見つかりませんでした。\n", node_id);
	}

	return UINT_MAX;
}

//指定されたピンIDが所属している親ノードのIDを逆引き取得
uint32_t GraphDataManager::GetNodeIdFromPinId(uint32_t graph_id, uint32_t pin_id)
{
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
		{
			if (layer_datas[g].nodes[n]->HasPin(pin_id))
			{
				return layer_datas[g].nodes[n]->GetNodeBasicData().id;
			}
		}
	}
	return UINT32_MAX;
}