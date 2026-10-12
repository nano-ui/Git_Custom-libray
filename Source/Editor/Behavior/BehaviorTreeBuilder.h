#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <string>

class BehaviorNode;
class SelectorNode;
class WeightedNode;
class EditorActionNode;
class StateBlackboard;
struct GraphData;
struct GraphNode;
struct GraphLink;
struct GraphTransitionCondition;

class BehaviorTreeBuilder
{
public:
	//コンストラクタ
	BehaviorTreeBuilder() = default;

	//デストラクタ
	~BehaviorTreeBuilder() = default;

	//ツリーを構築
	std::shared_ptr<BehaviorNode> BuildTree(
		const GraphData* graph,
		StateBlackboard* blackboard,
		std::function<void(uint32_t, const std::string&, bool, bool)>action_callback
	);

private:
	//優先順位ノード作成
	std::shared_ptr<SelectorNode> CreateSelectorNode(const GraphNode& node);

	//重み抽選ノード作成
	std::shared_ptr<WeightedNode> CreateWeightedNode(const GraphNode& node);

	//アクションノード生成
	std::shared_ptr<EditorActionNode> CreateActionNode(
		const GraphNode& node,
		std::function<void(uint32_t, const std::string&, bool, bool)>action_callback
	);

	//ノード間のリンクを検索
	const GraphLink* FindLinkBetweenNodes(const GraphData* graph, uint32_t parent_node_id, uint32_t child_node_id);

	//評価条件処理
	bool EvaluateConditions(const std::vector<GraphTransitionCondition>& conditions, StateBlackboard* blackboard);

	//ルートノードを検索
	uint32_t FindRootNodeId(const GraphData* graph);

	//子ノード群を取得
	std::vector<uint32_t> GetChildNodeIds(const GraphData* graph, uint32_t node_id);

	//グラフノードを検索
	const GraphNode* FindGraphNode(const GraphData* graph, uint32_t node_id);
};

