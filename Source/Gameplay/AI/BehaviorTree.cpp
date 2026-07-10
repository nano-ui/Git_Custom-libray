#include "BehaviorTree.h"
#include "BehaviorNode.h"
#include "SelectorNode.h"
#include "WeightedNode.h"

//=============
//ツリー更新
//=============
void BehaviorTree::UpdateTree(float elapsed_time)
{
	//ルートノードが存在するか確認
	if (tree_root)
	{
		tree_root->Execute(elapsed_time);	//ルートノード実行
	}
}

//=========================
//ルートノードの追加
//=========================
bool BehaviorTree::AddRootNode
(
	CompositeNodeID entry_id
)
{
	if (tree_root)return false;	//既にルートが存在する場合は追加をしない
	tree_root = std::make_shared<SelectorNode>(static_cast<int>(entry_id));	//指定IDでセレクターをルートとして追加
	return true;	//登録完了
}

//============================
//セレクターノードの追加
//============================
bool BehaviorTree::AddSelectorNode
(
	CompositeNodeID parent_id,			//親ノードID
	CompositeNodeID entry_id,			//生成するノードID
	std::function<bool()> condition		//実行許可関数
)
{
	auto parent_node = FindParentNode(parent_id);	//IDをもとに親となインスタンスを取得
	if (!parent_node) return false;					//親が見つからない場合は処理を中断
	auto selector = std::dynamic_pointer_cast<SelectorNode>(parent_node);	//親がセレクター形式か確認
	if (!selector)return false;		//セレクター以外なら接続拒否
	auto new_node = std::make_shared<SelectorNode>(static_cast<int>(entry_id));	//指定のIDでセレクターを生成
	if (condition) new_node->SetCondition(condition);	//条件がある場合はノードに追加
	new_node->SetParent(parent_node);	//作成したノードに親ノードを登録
	selector->AddChildNode(new_node);	//子ノードに追加
	return true;	//登録完了
}

//============================
//ウェイトノードの追加
//============================
bool BehaviorTree::AddWeightedNode
(
	CompositeNodeID parent_id,		//親ノードID
	CompositeNodeID entry_id,		//生成するノードID
	std::function<bool()> condition	//実行許可関数
)
{
	auto parent_node = FindParentNode(parent_id);	//IDをもとに親となインスタンスを取得
	if (!parent_node) return false;					//親が見つからない場合は処理を中断
	auto selector = std::dynamic_pointer_cast<SelectorNode>(parent_node);	//親がセレクター形式か確認
	if (!selector)return false;		//セレクター以外なら接続拒否
	auto new_node = std::make_shared<WeightedNode>(static_cast<int>(entry_id));	//指定のIDでセレクターを生成
	if (condition) new_node->SetCondition(condition);	//条件がある場合はノードに追加
	new_node->SetParent(parent_node);	//作成したノードに親ノードを登録
	selector->AddChildNode(new_node);	//子ノードに追加
	return true;	//登録完了
}

//============================
//アクションノードの追加
//============================
bool BehaviorTree::AddActionNode
(
	CompositeNodeID parent_id,	//親ノードID
	ActionNodeID entry_id,		//アクションの識別ID
	std::shared_ptr<BehaviorNode> action_node,	//実行されるアクションのインスタンス
	std::function<bool()> condition,	//実行許可関数
	int weight_value					//親がウェイトの時の重み
)
{
	if (!action_node)return false;						//アクション本体が存在しないなら追加拒否
	auto parent_node = FindParentNode(parent_id);		//接続先の親ノードを取得
	if (!parent_node) return false;						//親が見つからなければ追加拒否
	if (condition)action_node->SetCondition(condition);	//インスタンスに判定条件を設定

	action_node->SetParent(parent_node);	//アクションノードに親を登録

	auto selector = std::dynamic_pointer_cast<SelectorNode>(parent_node);	//親がセレクターか検証
	if (selector)	//セレクターの場合
	{
		selector->AddChildNode(action_node);	//子ノードとして登録
		return true;							//登録完了
	}

	auto weighted = std::dynamic_pointer_cast<WeightedNode>(parent_node);	//親がウェイトか判定
	if (weighted)	//ウェイトの場合
	{
		weighted->AddChildNode(action_node, weight_value);	//子ノードとして登録
		return true;										//登録完了
	}

	return false;	//登録失敗
}

//===================
//親ノード検索
//===================
std::shared_ptr<BehaviorNode> BehaviorTree::FindParentNode
(
	CompositeNodeID parent_id	//検索するノード
)
{
	if (!tree_root)return nullptr;	//ツリーが構築されていなければ検索不可

	if (static_cast<CompositeNodeID>(tree_root->GetID()) == parent_id)	//ルート自身が目的のノードか確認
	{
		return tree_root;	//ルートのインスタンスを返す
	}

	return tree_root->SearchNode(static_cast<int>(parent_id));	//ノードを検索
}
