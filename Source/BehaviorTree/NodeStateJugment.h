#pragma once
#include "JudgmentNode.h"
#include "BehaviorTree.h"

#include <memory>
#include <vector>

class NodeStateJugment : public JudgmentNode
{
public:
	//NodeStateJugment(std::shared_ptr<BehaviorTree>tree, const std::vector<NodeId>& target_node_ids);

	//判定処理
	bool Check() override;
private:
	std::shared_ptr<BehaviorTree> behavior_tree;	//監視対象のツリー
	//std::vector<NodeId> node_ids;					//指定したノード群
};

