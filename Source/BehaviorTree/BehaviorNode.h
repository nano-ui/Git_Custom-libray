#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "BehaviorTree.h"

//ノードの実行状態
enum class NodeState
{
	Success,	//行動成功
	Failure,	//行動失敗
	Running		//行動中
};

class BehaviorNode
{
public:
	BehaviorNode(int node_id) :id(node_id){}

	//仮想デストラクタ
	virtual ~BehaviorNode() = default;

	//ビヘイビアツリー実行関数
	virtual NodeState Execute(float elapsed_time) = 0;

	//ID(int)でノードを検索
	virtual std::shared_ptr<BehaviorNode> SearchNode(int search_id) = 0;

	//自身が実行可能かどうかを判定
	virtual bool CanExecute()const;

public:
	//外部から判定条件の関数を設定
	void SetCondition(std::function<bool()>cond_func) { condition = cond_func; }

	//ID取得
	int GetID()const { return id; }

	//親ノードを設定
	void SetParent(std::shared_ptr<BehaviorNode> parent_ptr) { parent = parent_ptr; }

	//親ノードを取得
	std::shared_ptr<BehaviorNode>GetParent()const { return parent.lock(); }

private:
	int id;	//ノードの識別ID
	std::function<bool()> condition = nullptr;	//実行可能かを判定するための関数を保持
	std::weak_ptr<BehaviorNode> parent;
};

