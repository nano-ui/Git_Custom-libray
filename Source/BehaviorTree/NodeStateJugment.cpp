#include "NodeStateJugment.h"

//==================
//コンストラクタ
//==================
NodeStateJugment::NodeStateJugment(std::shared_ptr<BehaviorTree> tree, const std::vector<NodeId>& target_node_ids)
	:behavior_tree(tree), node_ids(target_node_ids)
{

}

//============
//判定処理
//============
bool NodeStateJugment::Check()
{
	if (!behavior_tree) return false;	//ビヘイビアツリーが存在するかチェック

	//現在のノードIDを取得してリストと照合
	//NodeId current_id = behavior_tree->GetCurrentNodeId();
	for (const auto& id : node_ids)
	{
		if (current_id == id) return true;
	}
	return false;
}
