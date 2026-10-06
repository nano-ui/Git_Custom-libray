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

//リンクの生成
bool GraphDataManager::CreateLink(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id)
{
	//接続の事前チェック
	if (!CheckCanConnect(graph_id, start_pin_id, end_pin_id))
	{
		return false;
	}

	//対象階層の特定
	GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)return false;

	//リンクの生成と登録
	GraphLink new_link;
	new_link.id = FetchAndIncrementId();
	new_link.start_pin_id = start_pin_id;
	new_link.end_pin_id = end_pin_id;
	graph_data->links.push_back(new_link);

	printf("GraphDataManager::CreateLink - 新規リンクを作成しました。リンクID: %u, 開始ピンID: %u -> 終了ピンID: %u\n",
		new_link.id,
		new_link.start_pin_id,
		new_link.end_pin_id);

	return true;
}

//リンクの削除
void GraphDataManager::DeleteLink(uint32_t graph_id, uint32_t target_link_id)
{
	GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)
	{
		printf("GraphDataManager::DeleteLink - 階層が見つかりませんでした。 階層ID: %u\n", graph_id);
		return;
	}

	for (auto it = graph_data->links.begin(); it != graph_data->links.end(); it++)
	{
		if (it->id == target_link_id)
		{
			it = graph_data->links.erase(it);
			printf("GraphDataManager: リンクを削除しました。ID: %d\n", target_link_id);
			return;
		}
	}
	printf("GraphDataManager::DeleteLink: 削除対象が存在しません。 ID: %d\n", target_link_id);
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
	GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)return UINT32_MAX;
	
	for (const auto& node : graph_data->nodes)
	{
		if ((*node).HasPin(pin_id))
		{
			return (*node).GetNodeBasicData().id;
		}
	}
	return UINT32_MAX;
}

//グラフ情報の取得
const GraphData* GraphDataManager::GetGraphData(uint32_t graph_id)const
{
	const GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)
	{
		printf("GraphDataManager::GetGraphData - 階層が見つかりませんでした。階層ID: %u\n", graph_id);
		return nullptr;
	}
	return graph_data;
}

//グラフ情報を検索
GraphData* GraphDataManager::FindGraphData(uint32_t graph_id)
{
	//階層配列の走査
	for (size_t l = 0; l < layer_datas.size(); l++)
	{
		if (graph_id == layer_datas[l].id)
		{
			return &layer_datas[l];
		}
	}
	return nullptr;
}

//グラフ情報を検索
const GraphData* GraphDataManager::FindGraphData(uint32_t graph_id) const
{
	//階層配列の走査
	for (size_t l = 0; l < layer_datas.size(); l++)
	{
		if (graph_id == layer_datas[l].id)
		{
			return &layer_datas[l];
		}
	}

	return nullptr;
}

//接続判定処理
bool GraphDataManager::CheckCanConnect(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id)
{
	//親ノードの特定
	uint32_t start_node_id = GetNodeIdFromPinId(graph_id, start_pin_id);
	uint32_t end_node_id = GetNodeIdFromPinId(graph_id, end_pin_id);

	if (start_node_id == UINT32_MAX || end_node_id == UINT32_MAX)
	{
		return false;
	}

	//自己ノード内接続の防止
	if (start_node_id == end_node_id)
	{
		return false;
	}

	GraphNode* start_node = nullptr;
	GraphNode* end_node = nullptr;

	//対象階層の特定とノード配列の走査
	GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)return false;

	for (const auto& it : graph_data->nodes)
	{
		if (start_node_id == (*it).GetNodeBasicData().id)
		{
			start_node = it.get();
		}

		if (end_node_id == (*it).GetNodeBasicData().id)
		{
			end_node = it.get();
		}
		if (start_node && end_node)break;
	}

	//ピンの属性取得とルール判定
	if (!start_node || !end_node)return false;

	PinType start_pin_type = start_node->GetPinType(start_pin_id);
	PinType end_pin_type = end_node->GetPinType(end_pin_id);

	if (start_pin_type == end_pin_type || start_pin_type == PinType::None || end_pin_type == PinType::None)return false;
	if (start_node->GetNodeBasicData().id == end_node->GetNodeBasicData().id)return false;
	if (start_pin_type == PinType::Input && end_pin_type == PinType::Output)return false;

	return true;
}

//ノードの追加
bool GraphDataManager::AddNode(uint32_t graph_id, std::unique_ptr<GraphNode> graph_node)
{
	//事前チェック
	if (!graph_node)return false;

	//対象階層の特定と存在検証
	GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)return false;

	//ノード配列への追加
	graph_data->nodes.push_back(std::move(graph_node));

	//完了通知と結果返却
	printf("GraphDataManager::AddNode - 新規ノードを作成。 追加先階層: %u ノードID: %u\n",
		graph_id,
		graph_data->nodes.back()->GetNodeBasicData().id);

	return true;
}

//ノードの削除
bool GraphDataManager::DeleteNode(uint32_t graph_id, uint32_t node_id)
{
	//階層データの取得と検証 
	GraphData* graph_data = FindGraphData(graph_id);
	if (!graph_data)return false;

	//削除対象ノードに接続されたリンクの特定と一括削除
	for (const auto& node : graph_data->nodes)
	{
		if (node_id == (*node).GetNodeBasicData().id)
		{
			std::vector<uint32_t> delete_link_id_list = {};
			for (const auto& link : graph_data->links)
			{
				if (((*node).HasPin(link.start_pin_id) || (*node).HasPin(link.end_pin_id)))
				{
					delete_link_id_list.push_back(link.id);
				}
			}
			for (const auto& target : delete_link_id_list)
			{
				DeleteLink(graph_id, target);
			}
		}
	}

	//ノード本体の削除と完了返却
	for (auto it = graph_data->nodes.begin(); it != graph_data->nodes.end(); it++)
	{
		if (node_id == (*it)->GetNodeBasicData().id)
		{
			graph_data->nodes.erase(it);
			printf("GraphDataManager::DeleteNode - 削除完了しました。 削除階層: %u ノードID: %u\n",
				graph_id,
				node_id);

			return true;
		}
	}

	//ノード非存在時のエラーハンドリング
	printf("GraphDataManager::DeleteNode - 削除対象が存在しません。削除階層: %u ノードID: %u\n",
		graph_id,
		node_id);

	return false;
}