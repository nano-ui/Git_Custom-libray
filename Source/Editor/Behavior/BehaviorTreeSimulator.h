#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "Editor\StateMachineEditor\StateGraphSimulator.h"

class StateBlackboard;
class BehaviorNode;
class BehaviorTreeBuilder;

class BehaviorTreeSimulator
{
public:
	//コンストラクタ
	BehaviorTreeSimulator();

	//デストラクタ
	~BehaviorTreeSimulator();

	//グラフ情報の設定
	void SetupGraph(
		const std::vector<SimulatorRuntimeNode>& nodes,
		const std::vector<SimulatorRuntimeLink>& links);

	//シミュレーション更新処理
	bool UpdateSimulation(float elapsed_time, StateBlackboard* blackboard);

	//ツリーの再構築
	void InvalidataTree() { return runtime_root_node.reset(); }

	//実行ノードID取得
	uint32_t GetCurrentActiveNodeId()const { return active_node_id; }

	//再生アニメーション名取得
	const std::string& GetCurrentAnimationName() { return current_animation_name; }

	//アニメーションループフラグ取得
	bool GetCurrentAnimationLoop() { return current_animation_loop; }

	//ルートモーションフラグ取得
	bool GetRootMotion() { return current_root_motion; }

private:
	//
	void OnActionExecuted(uint32_t node_id, const std::string& anim_name, bool loop, bool root_motion);

private:
	std::unique_ptr<BehaviorTreeBuilder> tree_builder;	//ツリー構築クラス
	std::shared_ptr<BehaviorNode> runtime_root_node;	//実行ノード

	std::vector<SimulatorRuntimeNode> current_runtime_nodes; //現在のグラフノード
	std::vector<SimulatorRuntimeLink> current_runtime_links; //現在のグラフリンク

	uint32_t active_node_id = UINT32_MAX;				//実行ノードID
	uint32_t running_action_node_id = UINT32_MAX;		//実行中ノードID
	float action_timer = 0.0f;							//実行時間
	std::string current_animation_name = "";			//再生アニメーション名
	bool current_animation_loop = true;					//アニメーションループフラグ
	bool current_root_motion = false;					//ルートモーションフラグ
};

