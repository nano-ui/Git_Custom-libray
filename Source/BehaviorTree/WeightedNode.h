#pragma once

#include <vector>
#include <memory>

#include "BehaviorNode.h"

//子ノードとその抽選割合を保持
struct ChildWeightedData
{
	std::shared_ptr<BehaviorNode> node;	//実行する子ノード
	int weight;							//ノードが選ばれる確率
};

class WeightedNode :public BehaviorNode
{
public:
	//基底クラスのコンストラクタを呼び出す
	WeightedNode(int node_id) :BehaviorNode(node_id) {};

	//子ノードと抽選確立を追加
	void AddChildNode(std::shared_ptr<BehaviorNode> child_node, int weight);

	//ノードを実行
	NodeState Execute(float elapsed_time) override;

	//名前でノードを検索
	std::shared_ptr<BehaviorNode> SearchNode(int search_id);

	//ノードの状態を初期化
	void ResetState();

private:
	std::vector<ChildWeightedData> child_nodes;	//子ノードと抽選確立を保持するリスト
	std::shared_ptr<BehaviorNode> active_node;	//現在実行中のノードを記憶
	int total_weight = 0;						//登録された確率の合計値
};

