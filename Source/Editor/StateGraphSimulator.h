#pragma once

#include <cstdint>

class StateBlackboard;
class StateGraphDataManager;
struct GraphData;

class StateGraphSimulator
{
public:
	//コンストラクタ
	StateGraphSimulator() = default;

	//デストラクタ
	~StateGraphSimulator() = default;

	//現在の階層におけるステートの遷移をブラックボードをもとに評価・更新
	bool UpdateSimulation(
		StateGraphDataManager* data_manager,
		StateBlackboard* blackboard,
		GraphData* current_graph,
		uint32_t& in_out_active_node_id,
		uint32_t& out_flowing_link_id
	);
};

