#pragma once

#include <memory>
#include <string>
#include <functional>

class BehaviorNode;
enum class NodeState;

//中間ノードの種類
enum class CompositeNodeID
{
	Node,	//親を持たない
	Root,	//思考の起点
	Battle,	//戦闘判断
	Attack,	//攻撃方法の抽選
	Scout,	//戦闘以外の行動
};

//末端ノードの種類
enum class ActionNodeID
{
	None,

	// 移動・待機系
	Move_Leave,     // 対象から距離をとる
	Move_Ran,       // 走りながら対象を追跡
	Move_Pursuit,   // 対象を追跡
	Move_Wander_L,  // 左回りで周囲を徘徊
	Move_Wander_R,  // 右回りで周囲を徘徊
	Move_Idle,      // 待機
	Move_Jump,      // ジャンプして移動

	// 攻撃系
	Attack_Heavy,   // 強攻撃
	Attack_Normal,  // 通常攻撃
	Attack_Quick,   // 弱攻撃
	Attack_Skill,   // 特殊攻撃

	// 状態異常系
	Dead,           // 死亡
	Down,           // ダウン
	Hesitation      // 怯み
};

class BehaviorTree
{
public:
	//ツリー更新
	void UpdateTree(float elapsed_time);

	//ルートノードを設定
	void SetRootNode(std::shared_ptr<BehaviorNode> root_node) { tree_root = root_node; }

	//ルートノードの追加
	bool AddRootNode(CompositeNodeID entry_id);

	//セレクターノードの追加
	bool AddSelectorNode(CompositeNodeID parent_id, CompositeNodeID entry_id, std::function<bool()> condition = nullptr);

	//ウェイトノードの追加
	bool AddWeightedNode(CompositeNodeID parent_id, CompositeNodeID entry_id, std::function<bool()> condition = nullptr);

	//アクションノードの追加
	bool AddActionNode(CompositeNodeID parent_id, ActionNodeID entry_id, std::shared_ptr<BehaviorNode> action_node, std::function<bool()>condition = nullptr, int weight_value = 1);


public:
	//実行中のノードIDを取得
	ActionNodeID GetCurrentNodeId()const { return current_node_id; }

	//実行中のノードIDを設定
	void SetCurrentNodeId(ActionNodeID node_id) { current_node_id = node_id; }

	//実行中のノードポインタを取得
	BehaviorNode* GetCurrentNodePtr()const { return current_node_ptr; }

	//実行中のノードポインタを設定
	void SetCurrentNodePtr(BehaviorNode* node) { current_node_ptr = node; }

	//ビヘイビアノードを取得
	BehaviorNode* GetBehaviorNode() { return tree_root.get(); }

private:
	//親ノード検索
	std::shared_ptr<BehaviorNode> FindParentNode(CompositeNodeID parent_id);

private:
	std::shared_ptr<BehaviorNode> tree_root;	//ツリーの頂点ルートノード
	ActionNodeID current_node_id = ActionNodeID::None;	//現在の実行アクションIDを記録
	BehaviorNode* current_node_ptr = nullptr;	//現在の実行ノードをインスタンスアドレスで記録
};

