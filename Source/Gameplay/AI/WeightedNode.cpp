#include "WeightedNode.h"

#include <random>

//=============================
//子ノードと抽選確率を追加
//=============================
void WeightedNode::AddChildNode(std::shared_ptr<BehaviorNode> child_node, int weight)
{
	//-------------------------------------
	//子ノードの登録と抽選確立の事前計算
	//-------------------------------------
	child_nodes.push_back({ child_node,weight }); //子ノードをリストに追加
	total_weight += weight;		//確率の合計値を加算
}

//=================
//ノードを実行
//=================
NodeState WeightedNode::Execute(float elapsed_time)
{
	//-------------------------
	//実行中のノード継続処理
	//-------------------------
	if (active_node)	//実行中のノードがあるか確認
	{
		NodeState current_state = active_node->Execute(elapsed_time);	//実行中のノードを進める
		if (current_state == NodeState::Running)	//処理が実行中か確認
		{
			return NodeState::Running;	//実行中として返す
		}
		active_node.reset();	//処理が完了したため、記録を消去
		return current_state;	//実行中の状態を返す
	}

	//---------------------
	//新しいノードの抽選
	//---------------------
	if (child_nodes.empty())	//子ノードが1つでも登録されているか確認
	{
		return NodeState::Failure;	//失敗状態を返す
	}
	//今「実行可能」な子ノードだけを集め、その時だけの「一時的な重み合計」を計算
	int current_total_weight = 0;
	std::vector<ChildWeightedData> valid_nodes;
	for (const auto& child_data : child_nodes)
	{
		if (child_data.node && child_data.node->CanExecute())
		{
			valid_nodes.push_back(child_data);
			current_total_weight += child_data.weight;
		}
	}
	if (valid_nodes.empty() || current_total_weight <= 0)	//もし実行可能なノードが1つも無い場合は失敗
	{
		return NodeState::Failure;
	}
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int>dist(0, current_total_weight - 1);

	int random_value = dist(gen);	//ランダム値を取得
	int current_weight_sum = 0;
	for (const auto& valid_data : valid_nodes)	//実行可能なノードの中から抽選
	{
		current_weight_sum += valid_data.weight;
		if (random_value < current_weight_sum)
		{
			active_node = valid_data.node;
			break;
		}
	}

	//-------------------------------
	//選択されたノードの初回実行
	//-------------------------------
	if (!active_node)	//ノードが選ばれているか確認
	{
		return NodeState::Failure;	//失敗状態を返す
	}
	NodeState initial_state = active_node->Execute(elapsed_time);	//選択されたノードを実行し結果を取得
	if (initial_state != NodeState::Running)	//初回でいきなり完了した場合は記録を消す
	{
		active_node.reset();	//記録をリセット
	}
	return initial_state;	//実行結果を返す
}

//=====================
//名前でノードを検索
//=====================
std::shared_ptr<BehaviorNode> WeightedNode::SearchNode(int search_id)
{
	//--------------
	//ノード検索
	//--------------
	for (auto& current_node : child_nodes)	//保持している子ノードを順番に検索
	{
		if (current_node.node->GetID() == search_id)	//子ノードのIDが一致するか確認
		{
			return current_node.node;	//一致したノードを返す
		}
		std::shared_ptr<BehaviorNode> found_node = current_node.node->SearchNode(search_id);	//その子ノードがさらに中間ノードであれば再帰的に検索
		if (found_node)
		{
			return found_node;	//見つかったノードを返す
		}
	}
	return nullptr;	//IDが一致しない場合
}

//========================
//ノードの状態を初期化
//========================
void WeightedNode::ResetState()
{
	active_node.reset();	//実行中のノードの記録をリセット
}
