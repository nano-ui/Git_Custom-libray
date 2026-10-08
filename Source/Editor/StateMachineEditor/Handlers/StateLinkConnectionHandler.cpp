#include "Editor\StateMachineEditor\Handlers\StateLinkConnectionHandler.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//接続先の作成クエリを検知してデータに追加
void StateLinkConnectionHandler::HandleLinkCreation(
	StateGraphDataManager* data_manager,	//データ管理インスタンス
	GraphData* current_graph,				//編集中のグラフデータ
	uint32_t current_graph_id)				//表示中の階層ID
{
	if (!data_manager || !current_graph)
	{
		printf("Error: StateLinkConnectionHandler::HandleLinkCreation - 引数が nullptr です。\n");
		return;
	}

	if (ed::BeginCreate())
	{
		ed::PinId start_pin_id;
		ed::PinId end_pin_id;
		if (ed::QueryNewLink(&start_pin_id, &end_pin_id))
		{
			const uint32_t start_id = static_cast<uint32_t>(start_pin_id.Get());
			const uint32_t end_id = static_cast<uint32_t>(end_pin_id.Get());

			if (CanConnect(data_manager, current_graph_id, start_id, end_id))
			{
				const ImVec4 success_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				constexpr float line_thickness = 2.0f;
				if (ed::AcceptNewItem(success_color, line_thickness))
				{
					GraphLink new_link;
					new_link.id = data_manager->FetchAndIncrementId();
					new_link.start_pin_id = start_id;
					new_link.end_pin_id = end_id;
					current_graph->links.push_back(new_link);
					OnLinkCreated(current_graph, new_link);
				}
			}
			else
			{
				const ImVec4 reject_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
				constexpr float reject_thickness = 2.0f;
				ed::RejectNewItem(reject_color, reject_thickness);
			}
		}
		ed::EndCreate();
	}
}

//ピン同士が接続ルールに準拠しているか判定
bool StateLinkConnectionHandler::CanConnect(
	StateGraphDataManager* data_manager,	//データ管理インスタンス
	uint32_t graph_id,						//階層ID
	uint32_t start_pin_id,					//接続元ピンID
	uint32_t end_pin_id)					//接続先ピンID
{
	const GraphNode* start_node = nullptr;
	const GraphNode* end_node = nullptr;
	PinType start_pin_type = PinType::None;
	PinType end_pin_type = PinType::None;

	const auto& layers = data_manager->GetGraphDatas();
	for (size_t g_idx = 0; g_idx < layers.size(); g_idx++)
	{
		if (layers[g_idx].id != graph_id) continue;

		for (size_t n_idx = 0; n_idx < layers[g_idx].nodes.size(); n_idx++)
		{
			const GraphNode* node = layers[g_idx].nodes[n_idx].get();
			if (!node) continue;

			if (node->HasPin(start_pin_id))
			{
				start_node = node;
				start_pin_type = node->GetPinType(start_pin_id);
			}
			if (node->HasPin(end_pin_id))
			{
				end_node = node;
				end_pin_type = node->GetPinType(end_pin_id);
			}
		}
		break;
	}

	if (!start_node || !end_node) return false;
	if (start_node->GetNodeBasicData().id == end_node->GetNodeBasicData().id) return false;
	if (start_pin_type == end_pin_type || start_pin_type == PinType::None || end_pin_type == PinType::None) return false;
	if (start_pin_type == PinType::Input && end_pin_type == PinType::Output) return false;

	return true;
}

//リンク作成完了時にピンから所属ノードを特定・検証
void StateLinkConnectionHandler::OnLinkCreated(
	const GraphData* current_graph,			//編集中のグラフデータ
	const GraphLink& new_link				//新規作成リンクデータ
)
{
	if (!current_graph) return;

	uint32_t source_node_id = 0;
	uint32_t target_node_id = 0;

	for (size_t n_idx = 0; n_idx < current_graph->nodes.size(); n_idx++)
	{
		const GraphNode* node = current_graph->nodes[n_idx].get();
		if (!node) continue;

		if (node->HasPin(new_link.start_pin_id))
		{
			source_node_id = node->GetNodeBasicData().id;
		}
		if (node->HasPin(new_link.end_pin_id))
		{
			target_node_id = node->GetNodeBasicData().id;
		}
	}

	if (source_node_id == 0 || target_node_id == 0)
	{
		printf("Error: StateLinkConnectionHandler::OnLinkCreated - 接続元または接続先ノードが見つかりません。\n");
		return;
	}

	printf("StateLinkConnectionHandler: リンク作成成功 ID: %u (ノード: %u -> %u)\n", new_link.id, source_node_id, target_node_id);
}
