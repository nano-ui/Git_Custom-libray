#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <memory>

class StateBlackboard;
struct TransitionCondition;

//実行時リンクデータ
struct SimulatorRuntimeLink
{
	uint32_t id;			//リンクID
	uint32_t start_pin_id;	//開始ピンID
	uint32_t end_pin_id;	//終了ピンID
	std::vector<TransitionCondition> conditions;	//遷移条件配列
};

//実行時ノードデータ
struct SimulatorRuntimeNode
{
	uint32_t id;						//ノードID
	std::string name;					//ノード名
	std::string animation_name;			//アニメーション名
	std::vector<uint32_t> inputs;		//入力ピンID配列
	std::vector<uint32_t> outputs;		//出力ピンID配列
	bool is_sub_graph = false;			//サブグラフフラグ
	uint32_t sub_graph_id;				//下位グラフID
	bool is_loop;						//ループフラグ
	bool is_root_motion = false;		//ルートモーションフラグ
	uint32_t parent_node_id = UINT_MAX;	//親サブグラフノードID
};

//ステートマシン共通シミュレーション
class StateGraphSimulator
{
public:
	//コンストラクタ
	StateGraphSimulator();

	//デストラクタ
	~StateGraphSimulator();

	//グラフデータのセットアップ
	void SetupRuntimeGraph(
		const std::vector<SimulatorRuntimeNode>& in_nodes,
		const std::vector<SimulatorRuntimeLink>& in_links,
		const std::unordered_map<uint32_t, uint32_t>& in_layer_entries
	);

	//更新処理
	bool UpdateSimulation(
		float elapsed_time,
		StateBlackboard* blackboard,
		bool is_anim_finished = false
	);

	//遷移確定時の通知コールバック登録
	void SetOnTransitionLinkCallback(const std::function<void(uint32_t)>& callback)
	{
		on_transition_link_callback = callback;
	}

	//アクティブノードID取得
	uint32_t GetCurrentNodeID()const { return current_node_id; }

	//アクティブノードIDの設定
	void SetCurreentNodeID(uint32_t node_id) { current_node_id = node_id; }

	//アニメーション名取得
	std::string GetAnimationName()const { return current_animation_name; }

	//アニメーションループフラグの取得
	bool GetAnimationLoop()const { return current_animation_loop; }

	//ルートモーション有効フラグ取得
	bool GetRootMotionEnabled()const;

private:
	//単一の遷移条件の評価
	bool EvaluateCondition(
		const TransitionCondition& cond,
		StateBlackboard* blackboard,
		bool is_anim_finished
	)const;

	//ピンIDから所属ノードIDを逆引き検索
	uint32_t GetNodeIdFromPinId(uint32_t pin_id)const;

	//ノードIDから所属サブグラフノードIDを逆引き検索
	uint32_t GetParentNodeId(uint32_t node_id)const;

private:
	uint32_t current_node_id = UINT32_MAX;									//現在のアクティブノード
	std::string current_animation_name = "";								//再生中のアニメーション名
	bool current_animation_loop = true;										//アニメーションループフラグ
	std::vector<SimulatorRuntimeNode> runtime_nodes;						//評価対象の全ノードリスト
	std::vector<SimulatorRuntimeLink> runtime_links;						//評価対象の全リンクリスト
	std::unordered_map<uint32_t, uint32_t> layer_entry_nades;				//階層ごとの先頭ノードIDマップ
	std::unordered_map<uint32_t, uint32_t> pin_to_node_map;					//ピンキャッシュマップ
	std::function<void(uint32_t)> on_transition_link_callback = nullptr;	//遷移成立時の通知先
};

