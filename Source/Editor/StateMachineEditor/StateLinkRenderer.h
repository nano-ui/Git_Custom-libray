#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

struct GraphData;
struct GraphLink;
struct GraphNode;
class StateGraphDataManager;

class StateLinkRenderer
{
public:
	StateLinkRenderer() = default;
	~StateLinkRenderer() = default;

	//階層内の全リンクとフローエフェクトを描画
	void DrawLinks(
		StateGraphDataManager* data_manager,
		const GraphData* current_graph,
		uint32_t flow_src_node_id,
		uint32_t flow_dst_node_id,
		float flow_timer
	);

private:
	//ピンIDから所属ノード情報と色を逆引きするための構造体
	struct PinCacheData
	{
		uint32_t node_id;	//所属ノードID
		float color_r;		//線の赤成分
		float color_g;		//線の緑成分
		float color_b;		//線の青成分
	};

	//グラフ内の全ノードからピンキャッシュを構築
	void BilidPinCache(
		const std::vector<GraphNode>& nodes,
		std::unordered_map<uint32_t, PinCacheData>& out_pin_cache_map
	);

	//単一リンクの描画と強調判定
	void DrawSingleLink(
		StateGraphDataManager* data_manager,
		const GraphLink& link,
		const std::unordered_map<uint32_t, PinCacheData>& pin_cache_map,
		uint32_t flow_src_node_id,
		uint32_t flow_dst_node_id,
		float flow_timer
	);

	//dst_node_id が target_active_id 自身、またはその親サブグラフであるかを判定
	bool IsMatchTransitionDestination(
		StateGraphDataManager* data_manager,
		uint32_t dst_node_id,
		uint32_t target_active_id
	);
};

