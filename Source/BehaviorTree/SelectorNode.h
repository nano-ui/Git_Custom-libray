#pragma once
#include "BehaviorNode.h"

#include <vector>

class SelectorNode :public BehaviorNode
{
public:
	//基底クラスのコンストラクタを呼び出す
	SelectorNode(int node_id):BehaviorNode(node_id){}

	//子ノードを追加
	void AddChildNode(std::shared_ptr<BehaviorNode> child_node);

	//ノードを実行
	NodeState Execute(float elapsed_time)override;

	//名前でノードを検索
	std::shared_ptr<BehaviorNode> SearchNode(int search_id);

private:
	std::vector<std::shared_ptr<BehaviorNode>> child_nodes;	//子ノードを保持するリスト
};

