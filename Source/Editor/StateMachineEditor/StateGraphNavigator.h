#pragma once

#include <cstdint>
#include <vector>
#include <string>

class StateGraphDataManager;
struct GraphData;

class StateGraphNavigator
{
public:
	StateGraphNavigator() = default;
	~StateGraphNavigator() = default;

	//上部のヘッダーのパンくずナビゲーションを描画し、階層遷移が発生したかを返す
	bool DrawHeaderNavigation(
		StateGraphDataManager* data_manager,
		uint32_t& in_out_graph_id
	);

	//サブグラフノードのダブルクリックによる階層移動を判定・更新
	void CheckNavigateToSubGraph(
		const GraphData* current_graph,
		uint32_t& in_out_graph_id
	);

private:
	//指定された階層IDからルー地までの親ノード階層リストを遡って構築
	std::vector<uint32_t> BuildBreadcrumbPath(
		StateGraphDataManager* data_manager,
		uint32_t state_graph_id
	);
};

