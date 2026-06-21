#pragma once

#include "BehaviorNode.h"
#include <memory>
#include <string>

template<typename TargetType>
class ActionNode:public BehaviorNode
{
public:
	ActionNode(int node_id,std::weak_ptr<TargetType>owner_)
 : BehaviorNode(node_id), owner(owner_) {}
	virtual ~ActionNode() = default;

	//ビヘイビアツリー実行
	virtual NodeState Execute(float elapsed_time) override = 0;

	virtual bool CanExecute()const = 0;

	virtual std::shared_ptr<BehaviorNode> SearchNode(int search_id) override
	{
		return nullptr;
	}

	//状態を初期化
	virtual void ResetState() { current_step = 0; }

protected:
	std::weak_ptr<TargetType> owner;	//操作対象
	int current_step = 0;	//現在の行動ステップを記録
};

