#include "SelectorNode.h"

//==================
//子ノードを追加
//==================
void SelectorNode::AddChildNode(std::shared_ptr<BehaviorNode> child_node)
{
	child_nodes.push_back(child_node);	//リストに子ノードを追加
}

//===============
//ノードを実行
//===============
NodeState SelectorNode::Execute(float elapsed_time)
{
	//-------------------
	//子ノードの評価
	//-------------------
	for (auto& current_node : child_nodes)	//所持している子ノードの順にループ
	{
		if (!current_node->CanExecute())	//子ノードが実行可能か確認する
		{
			continue;	//条件を満たしていない場合はスキップ
		}
		NodeState node_state = current_node->Execute(elapsed_time);	//子ノードを実行する
		if (node_state == NodeState::Running)	//実行状態か判定
		{
			return NodeState::Running;	//現在も実行中として返す
		}
		else if (node_state == NodeState::Success)	//子ノードが成功状態か判定
		{
			return NodeState::Success;	//目的達成したため成功と返す
		}
	}
	return NodeState::Failure;	//全ての子ノードが失敗した場合は失敗状態を返す
}

//=======================
//名前でノードを検索
//=======================
std::shared_ptr<BehaviorNode> SelectorNode::SearchNode(int search_id)
{
	//--------------
	//ノード検索
	//--------------
	for (auto& current_node : child_nodes)	//保持している子ノードを順番に検索
	{
		if (current_node->GetID() == search_id)	//子ノードのIDが一致するか確認
		{
			return current_node;	//一致したノードを返す
		}
		std::shared_ptr<BehaviorNode> found_node = current_node->SearchNode(search_id);	//その子ノードがさらに中間ノードであれば再帰的に検索
		if (found_node)
		{
			return found_node;	//見つかったノードを返す
		}
	}
	return nullptr;	//IDが一致しない場合
}
