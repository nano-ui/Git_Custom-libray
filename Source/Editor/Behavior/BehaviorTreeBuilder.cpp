#include "BehaviorTreeBuilder.h"
#include "Editor\Behavior\EditorActionNode.h"
#include "Gameplay\AI\SelectorNode.h"
#include "Gameplay\AI\WeightedNode.h"
#include "Gameplay\StateMachine\StateBlackboard.h"

//ツリーを構築
std::shared_ptr<BehaviorNode> BehaviorTreeBuilder::BuildTree(
	const std::vector<SimulatorRuntimeNode>& runtime_nodes,
	const std::vector<SimulatorRuntimeLink>& runtime_links,
	std::function<void(uint32_t, const std::string&, bool, bool)> action_callback)
{
	if (runtime_nodes.empty())
	{
		return nullptr;
	}

	uint32_t root_id = FindRootNodeId(runtime_nodes);
	if (root_id == UINT32_MAX)
	{
		printf("Warning: BehaviorTreeBuilder::BuildTree - Rootノードが存在しません。\n");
		return nullptr;
	}

	std::unordered_map<uint32_t, std::shared_ptr<BehaviorNode>> created_nodes;

	for (size_t n = 0; n < runtime_nodes.size(); n++)
	{
		const SimulatorRuntimeNode& node = runtime_nodes[n];
		if (node.behavior_data.category == BehaviorCategory::Root)
		{
			created_nodes[node.id] = std::make_shared<SelectorNode>(static_cast<int>(node.id));
		}
		else if (node.behavior_data.category == BehaviorCategory::Composite)
		{
			if (node.behavior_data.composite_node_type == CompositeNodeType::Select)
			{
				created_nodes[node.id] = CreateSelectorNode(node);
			}
			else
			{
				created_nodes[node.id] = CreateWeightedNode(node);
			}
		}
		else if (node.behavior_data.category == BehaviorCategory::Action)
		{
			created_nodes[node.id] = CreateActionNode(node, action_callback);
		}
	}

	for (size_t n = 0; n < runtime_nodes.size(); n++)
	{
		const SimulatorRuntimeNode& parent_node = runtime_nodes[n];
		auto parent_it = created_nodes.find(parent_node.id);
		if (parent_it == created_nodes.end())
		{
			continue;
		}

		std::vector<uint32_t> child_ids = GetChildNodeIds(runtime_nodes, runtime_links, parent_node.id);
		for (size_t c = 0; c < child_ids.size(); c++)
		{
			auto child_it = created_nodes.find(child_ids[c]);
			if (child_it == created_nodes.end())
			{
				continue;
			}
			const SimulatorRuntimeNode* child_graph_node = FindGraphNode(runtime_nodes, child_ids[c]);
			int weight = (child_graph_node && child_graph_node->behavior_data.weight > 0) ? child_graph_node->behavior_data.weight : 1;

			const uint32_t current_parent_id = parent_node.id;
			const uint32_t current_child_id = child_ids[c];

			child_it->second->SetCondition([this, runtime_nodes, runtime_links, current_parent_id, current_child_id]() -> bool
				{
					const SimulatorRuntimeLink* current_link = this->FindLinkBetweenNodes(runtime_nodes, runtime_links, current_parent_id, current_child_id);
					if (!current_link || current_link->conditions.empty())
					{
						return true;
					}
					return this->EvaluateConditions(current_link->conditions, this->active_blackboard);
				});

			auto selector = std::dynamic_pointer_cast<SelectorNode>(parent_it->second);
			if (selector)
			{
				selector->AddChildNode(child_it->second);
			}

			auto weighted = std::dynamic_pointer_cast<WeightedNode>(parent_it->second);
			if (weighted)
			{
				weighted->AddChildNode(child_it->second, weight);
			}
		}
	}
	return created_nodes[root_id];
}

//優先順位ノード作成
std::shared_ptr<SelectorNode> BehaviorTreeBuilder::CreateSelectorNode(const SimulatorRuntimeNode& node)
{
	return std::make_shared<SelectorNode>(static_cast<int>(node.id));
}

//重み抽選ノード作成
std::shared_ptr<WeightedNode> BehaviorTreeBuilder::CreateWeightedNode(const SimulatorRuntimeNode& node)
{
	return std::make_shared<WeightedNode>(static_cast<int>(node.id));
}

//アクションノード生成
std::shared_ptr<EditorActionNode> BehaviorTreeBuilder::CreateActionNode(
	const SimulatorRuntimeNode& node,
	std::function<void(uint32_t, const std::string&, bool, bool)> action_callback)
{
	return std::make_shared<EditorActionNode>(
		static_cast<int>(node.id),
		node.animation_name,
		node.is_loop,
		node.is_root_motion,
		node.behavior_data.action_duration,
		node.behavior_data.result_type,
		action_callback);
}

//ノード間のリンクを検索
const SimulatorRuntimeLink* BehaviorTreeBuilder::FindLinkBetweenNodes(
	const std::vector<SimulatorRuntimeNode>& runtime_nodes,
	const std::vector<SimulatorRuntimeLink>& runtime_links,
	uint32_t parent_node_id,
	uint32_t child_node_id)
{
	const SimulatorRuntimeNode* parent = FindGraphNode(runtime_nodes, parent_node_id);
	const SimulatorRuntimeNode* child = FindGraphNode(runtime_nodes, child_node_id);
	if (!parent || !child)
	{
		return nullptr;
	}

	for (size_t l = 0; l < runtime_links.size(); l++)
	{
		const SimulatorRuntimeLink& link = runtime_links[l];
		for (size_t op = 0; op < parent->outputs.size(); op++)
		{
			if (link.start_pin_id == parent->outputs[op])
			{
				for (size_t ip = 0; ip < child->inputs.size(); ip++)
				{
					if (link.end_pin_id == child->inputs[ip])
					{
						return &link;
					}
				}
			}
		}
	}
	return nullptr;
}

//評価条件処理
bool BehaviorTreeBuilder::EvaluateConditions(const std::vector<TransitionCondition>& conditions, StateBlackboard* blackboard) 
{
	if (!blackboard)
	{
		return true;
	}

	for (const auto& cond : conditions)
	{
		if (!cond.IsJudgment(*blackboard))
		{
			return false;
		}
	}

	return true;
}

//ルートノードを検索
uint32_t BehaviorTreeBuilder::FindRootNodeId(const std::vector<SimulatorRuntimeNode>& runtime_nodes) 
{
	//階層内のノードを巡回しルートノードを検索
	for (size_t n = 0; n < runtime_nodes.size(); n++)
	{
		if (runtime_nodes[n].behavior_data.category == BehaviorCategory::Root)
		{
			return runtime_nodes[n].id;
		}
	}

	return UINT32_MAX;
}

//子ノード群を取得
std::vector<uint32_t> BehaviorTreeBuilder::GetChildNodeIds(
	const std::vector<SimulatorRuntimeNode>& runtime_nodes,
	const std::vector<SimulatorRuntimeLink>& runtime_links,
	uint32_t node_id)
{
	std::vector<uint32_t> children;	//下位ノードIDリスト

	const SimulatorRuntimeNode* parent = FindGraphNode(runtime_nodes, node_id);	//親ノード
	if (!parent)
	{
		return children;
	}

	//対象のノードの接続先ノードIDを検索
	for (size_t p = 0; p < parent->outputs.size(); p++)	//親ノードの全出力ピンを巡回
	{
		const uint32_t out_pin_id = parent->outputs[p];	//対象の出力ピン
		for (size_t l = 0; l < runtime_links.size(); l++)	//グラフ内の全リンクを巡回
		{
			if (runtime_links[l].start_pin_id == out_pin_id)	//対象の出力ピンと探索中の出力ピンが一致するか判定
			{
				const uint32_t in_pin_id = runtime_links[l].end_pin_id;	//接続先の入力ピン
				for (size_t n = 0; n < runtime_nodes.size(); n++)	//グラフ内の全ノードを巡回
				{
					for (size_t ip = 0; ip < runtime_nodes[n].inputs.size(); ip++)	//探索中の全入力ピンを巡回
					{
						if (runtime_nodes[n].inputs[ip] == in_pin_id)	//接続先の入力ピンと探索中の入力ピンが一致するか判定
						{
							children.push_back(runtime_nodes[n].id);	//下位ノードリストに追加
							break;
						}
					}
				}
			}
		}
	}
	return children;
}

//グラフノードを検索
const SimulatorRuntimeNode* BehaviorTreeBuilder::FindGraphNode(const std::vector<SimulatorRuntimeNode>& runtime_nodes, uint32_t node_id)
{
	//階層内のノード群を巡回し対象のIDを検索
	for (size_t n = 0; n < runtime_nodes.size(); n++)
	{
		if (runtime_nodes[n].id == node_id)
		{
			return &runtime_nodes[n];
		}
	}

	return nullptr;
}