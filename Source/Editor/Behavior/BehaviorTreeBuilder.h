#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <string>

#include "Editor\StateMachineEditor\StateGraphSimulator.h"
#include "Gameplay\StateMachine\StateBlackboard.h"

class BehaviorNode;
class SelectorNode;
class WeightedNode;
class EditorActionNode;
class StateBlackboard;

struct TransitionCondition;

class BehaviorTreeBuilder
{
public:
	//コンストラクタ
	BehaviorTreeBuilder() = default;

	//デストラクタ
	~BehaviorTreeBuilder() = default;

	//ツリーを構築
	std::shared_ptr<BehaviorNode> BuildTree(
		const std::vector<SimulatorRuntimeNode>& runtime_nodes,
		const std::vector<SimulatorRuntimeLink>& runtime_links,
		std::function<void(uint32_t, const std::string&, bool, bool)>action_callback
	);

	//ブラックボードの設定
	void SetActiveBlackboard(StateBlackboard* blackboard) { active_blackboard = blackboard; }

private:
	//優先順位ノード作成
	std::shared_ptr<SelectorNode> CreateSelectorNode(const SimulatorRuntimeNode& node);

	//重み抽選ノード作成
	std::shared_ptr<WeightedNode> CreateWeightedNode(const SimulatorRuntimeNode& node);

	//アクションノード生成
	std::shared_ptr<EditorActionNode> CreateActionNode(
		const SimulatorRuntimeNode& node,
		std::function<void(uint32_t, const std::string&, bool, bool)>action_callback
	);

	//ノード間のリンクを検索
	const SimulatorRuntimeLink* FindLinkBetweenNodes(
		const std::vector<SimulatorRuntimeNode>& runtime_nodes,
		const std::vector<SimulatorRuntimeLink>& runtime_links,
		uint32_t parent_node_id,
		uint32_t child_node_id);

	//子ノード群を取得
	std::vector<uint32_t> GetChildNodeIds(
		const std::vector<SimulatorRuntimeNode>& runtime_nodes,
		const std::vector<SimulatorRuntimeLink>& runtime_links,
		uint32_t node_id);

	//評価条件処理
	bool EvaluateConditions(const std::vector<TransitionCondition>& conditions, StateBlackboard* blackboard);

	//ルートノードを検索
	uint32_t FindRootNodeId(const std::vector<SimulatorRuntimeNode>& runtime_nodes);

	//グラフノードを検索
	const SimulatorRuntimeNode* FindGraphNode(const std::vector<SimulatorRuntimeNode>& runtime_nodes, uint32_t node_id);

private:
	StateBlackboard* active_blackboard = nullptr;	//有効なブラックボード
};

