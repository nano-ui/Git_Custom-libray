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
struct GraphData;
struct GraphNode;

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

	//ルートノードを検索
	uint32_t FindRootNodeId(const GraphData* graph);

	//子ノード群を取得
	std::vector<uint32_t> GetChildNodeIds(const GraphData* graph, uint32_t node_id);

	//グラフノードを検索
	const GraphNode* FindGraphNode(const GraphData* graph, uint32_t node_id);
};

