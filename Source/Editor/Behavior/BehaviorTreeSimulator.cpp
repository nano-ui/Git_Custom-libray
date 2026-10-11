#include "BehaviorTreeSimulator.h"
#include "BehaviorTreeBuilder.h"
#include "Gameplay/AI/BehaviorNode.h"

#include <cstdio>

//コンストラクタ
BehaviorTreeSimulator::BehaviorTreeSimulator()
	: tree_builder(std::make_unique<BehaviorTreeBuilder>())
	, runtime_root_node(nullptr)
	, current_graph_data(nullptr)
	, active_node_id(UINT32_MAX)
{
}

//デストラクタ
BehaviorTreeSimulator::~BehaviorTreeSimulator() = default;

//グラフ情報の設定
void BehaviorTreeSimulator::SetupGraph(const GraphData* graph)
{
	if (current_graph_data == graph && runtime_root_node)
	{
		return;
	}

	current_graph_data = graph;
	active_node_id = UINT32_MAX;
	current_animation_name = "";

	if (tree_builder)
	{
		runtime_root_node = tree_builder->BuildTree(
			current_graph_data,
			[this](uint32_t id, const std::string& anim, bool loop, bool root_motion)
			{
				this->OnActionExecuted(id, anim, loop, root_motion);
			});
	}
	else
	{
		printf("Error: BehaviorTreeSimulator::SetupGraph - tree_builder が nullptr です。\n");
	}
}

//シミュレーション更新処理
bool BehaviorTreeSimulator::UpdateSimulation(float elapsed_time, StateBlackboard* blackboard)
{
	if (!runtime_root_node)
	{
		return false;
	}

	runtime_root_node->Execute(elapsed_time);
	return true;
}

void BehaviorTreeSimulator::OnActionExecuted(uint32_t node_id, const std::string& anim_name, bool loop, bool root_motion)
{
	active_node_id = node_id;
	current_animation_name = anim_name;
	current_animation_loop = loop;
	current_root_motion = root_motion;
}
