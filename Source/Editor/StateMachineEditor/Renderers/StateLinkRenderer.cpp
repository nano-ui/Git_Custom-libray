#include "Editor\StateMachineEditor\Renderers\StateLinkRenderer.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//階層内の全リンクとフローエフェクトを描画
void StateLinkRenderer::DrawLinks(
	StateGraphDataManager* data_manager,
	const GraphData* current_graph,
	uint32_t flow_src_node_id,
	uint32_t flow_dst_node_id,
	float flow_timer)
{
	if (!current_graph)
	{
		printf("Error: StateLinkRenderer::DrawLinks - current_graph が nullptr です。\n");
		return;
	}

	std::unordered_map<uint32_t, PinCacheData> pin_cache_map;
	BilidPinCache(current_graph->nodes, pin_cache_map);

	//階層内の全リンクを巡回して描画
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		DrawSingleLink(data_manager, current_graph->links[i], pin_cache_map, flow_src_node_id, flow_dst_node_id, flow_timer);
	}
}

//グラフ内の全ノードからピンキャッシュを構築
void StateLinkRenderer::BilidPinCache(
	const std::vector<std::unique_ptr<GraphNode>>& nodes,
	std::unordered_map<uint32_t, PinCacheData>& out_pin_cache_map)
{
	for (size_t n = 0; n < nodes.size(); n++)
	{
		const GraphNode* base_node = nodes[n].get();
		if (!base_node)continue;


		PinCacheData cache;
		cache.node_id = base_node->GetNodeBasicData().id;

		const StateGraphNode* state_node = dynamic_cast<const StateGraphNode*>(base_node);
		if (state_node)
		{
			const DirectX::XMFLOAT3 color = state_node->GetLinkColor();
			cache.color_r = color.x;
			cache.color_g = color.y;
			cache.color_b = color.z;
		}

		const auto& input_pins = base_node->GetInputPins();
		for (size_t p = 0; p < input_pins.size(); p++)
		{
			out_pin_cache_map[input_pins[p].pin_id] = cache;
		}

		const auto& output_pins = base_node->GetOutputPins();
		for (size_t p = 0; p < output_pins.size(); p++)
		{
			out_pin_cache_map[output_pins[p].pin_id] = cache;
		}
	}
}

//単一リンクの描画と強調判定
void StateLinkRenderer::DrawSingleLink(
	StateGraphDataManager* data_manager,
	const GraphLink& link,
	const std::unordered_map<uint32_t, PinCacheData>& pin_cache_map,
	uint32_t flow_src_node_id,
	uint32_t flow_dst_node_id,
	float flow_timer)
{
	constexpr float default_color_val = 1.0f;
	float color_r = default_color_val;
	float color_g = default_color_val;
	float color_b = default_color_val;
	uint32_t src_node_id = 0;

	auto start_it = pin_cache_map.find(link.start_pin_id);
	if (start_it != pin_cache_map.end())
	{
		src_node_id = start_it->second.node_id;
		color_r = start_it->second.color_r;
		color_g = start_it->second.color_g;
		color_b = start_it->second.color_b;
	}

	uint32_t dst_node_id = 0;
	auto end_it = pin_cache_map.find(link.end_pin_id);
	if (end_it != pin_cache_map.end())
	{
		dst_node_id = end_it->second.node_id;
	}

	bool is_active_transition = false;

	// タイマーが有効かつ、遷移元・遷移先が適合するか判定
	if (flow_timer > 0.0f)
	{
		if (src_node_id == flow_src_node_id)
		{
			// 直接一致、またはサブグラフ内部ノードへの潜り込み一致を判定
			if (IsMatchTransitionDestination(data_manager, dst_node_id, flow_dst_node_id))
			{
				is_active_transition = true;
				printf("StateLinkRenderer: 【強調成功】 リンクID:%u (ノード %u -> 接続先 %u [最終到達 %u])\n",
					link.id, src_node_id, dst_node_id, flow_dst_node_id);
			}
		}
	}

	constexpr float normal_thickness = 2.0f;	//通常時の太さ定数
	constexpr float highlight_thickness = 6.0f;	//強調時の太さ定数

	ImVec4 link_color = ImVec4(color_r, color_g, color_b, 1.0f);
	float thickness = normal_thickness;

	if (is_active_transition)
	{
		// 一瞬だけ明るく発光させる色
		link_color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
		thickness = highlight_thickness;
	}

	//リンク描画
	ed::Link(link.id, link.start_pin_id, link.end_pin_id, link_color, thickness);
}

//dst_node_id が target_active_id 自身、またはその親サブグラフであるかを判定
bool StateLinkRenderer::IsMatchTransitionDestination(
	StateGraphDataManager* data_manager,
	uint32_t dst_node_id,
	uint32_t target_active_id)
{
	if (dst_node_id == target_active_id)
	{
		return true;
	}
	if (!data_manager)
	{
		return false;
	}

	uint32_t target_graph_id = data_manager->GetGraphIdFromNodeId(target_active_id);
	if (target_active_id == UINT32_MAX || target_active_id == 0)
	{
		return false;
	}

	const auto& layers = data_manager->GetGraphDatas();
	for (size_t g = 0; g < layers.size(); g++)
	{
		for (size_t n = 0; n < layers[g].nodes.size(); n++)
		{
			const GraphNode* node = layers[g].nodes[n].get();
			if (!node)
			{
				continue;
			}

			const NodeBasicData basic_data = node->GetNodeBasicData();
			if (basic_data.is_sub_graph && basic_data.sub_graph_id == target_graph_id)
			{
				if (basic_data.id == dst_node_id)
				{
					return true;
				}
				return IsMatchTransitionDestination(data_manager, dst_node_id, basic_data.id);
			}
		}
	}

	return false;
}
