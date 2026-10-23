#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>

#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Editor\StateMachineEditor\StateGraphSimulator.h"
#include "Editor\StateMachineEditor\StateMachineDataLoader.h"
#include "Editor\Behavior\BehaviorTreeSimulator.h"
#include "Editor\StateMachineEditor\StateGraphSimulator.h"

class StateBlackboard;
class JsonSerializer;
class AnimationComponent;

class StateMachineComponent
{
public:
	static constexpr float DEFAULT_STATE_BLEND_TIME = 0.2f;

public:
	//コンストラクタ
	StateMachineComponent();

	//デストラクタ
	~StateMachineComponent();

	//初期化
	void Initialize(StateBlackboard* blackboard);

	//更新
	void Update(float elapsed_time, StateBlackboard* blackboard, bool is_anim_finished = false);

	//モデル名設定
	void SetModelName(const std::string& model_name);

	//ステートマシンパス設定
	void SetStateMachinePath(const std::string& path) { state_machine_path = path; }

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer);

	//モデルハッシュの設定
	void SetModelHash(uint32_t hash) { model_hash = hash; }

	//モデルハッシュの取得
	uint32_t GetModelHash()const { return model_hash; }

	//ステートマシンJSONパス取得
	const std::string& GetStateMachinePath()const { return state_machine_path; }

	//アニメーション対応表を取得
	const std::unordered_map<uint32_t, std::string>& GetAnimationMap()const { return animation_map; }

	//ステートID取得
	uint32_t GetCurrentNodeId()const { return current_node_id; }

	//アニメーションを抽出
	const std::string& GetCurrentAnimationName()const { return current_animation_name; }

	//ブレンド時間を取得
	float GetCurrentBlendTime()const { return current_blend_time; }
	 
	//アニメーションループを取得
	bool GetAnimationLoop()const { return current_animation_loop; }

	//アクションカテゴリを取得
	ActionCategory GetCurrentActionCategory() const;

	//現在のステートがルートモーションを有効にしているか取得
	bool IsCurrentRootMotionEnbled()const;

	//リフレッシュ要求
	void RequestReload() { is_pending_reload = true; }

private:

	//単一の遷移条件が成立しているか判定
	bool EvaluateCondition(const TransitionCondition& cond, StateBlackboard* blackboard, bool is_anim_finished)const;

	//ステートマシンJSONからアニメーション対応表を抽出
	void LoadAnimationMap(StateBlackboard* blackboard);

	//ピンIDから所属するノードIDを逆引き検索
	uint32_t GetNodeIdFromPinId(uint32_t pin_id)const;

	//指定されたノードIDの親サブグラフノードIDを逆引き
	uint32_t GetParentNodeId(uint32_t node_id)const;

	//ノードIDからグラフIDを取得
	uint32_t GetGraphIdFromNodeId(uint32_t node_id)const;

private:
	std::unique_ptr<StateGraphSimulator> state_graph_simulator;	//ステートマシン共通シミュレータ
	std::unique_ptr<BehaviorTreeSimulator> behavior_simulator;	//ビヘイビアツリーシミュレータ

	std::string state_machine_path = "";						//Jsonパス
	uint32_t model_hash = 0;									//モデルのハッシュ値
	uint32_t current_node_id = UINT32_MAX;						//アクティブノード
	float current_blend_time = DEFAULT_STATE_BLEND_TIME;		//ブレンド時間
	std::string current_animation_name = "";					//アニメーション名
	bool current_animation_loop;								//アニメーション再生フラグ
	bool is_pending_reload = false;								//リロード予約フラグ
	std::unordered_map<uint32_t, std::string> animation_map;	//ステートIDとアニメーション名の対応表
	std::vector<SimulatorRuntimeNode> runtime_nodes;			//ノード配列
	std::vector<SimulatorRuntimeLink> runtime_links;			//リンク配列

	std::unordered_map<uint32_t, uint32_t> layer_entry_nodes;	//各階層ごとの先頭エントリーIDマップ
	std::unordered_map<uint32_t, RuntimeLayerType> layer_types;

	std::unordered_map<uint32_t, std::vector<SimulatorRuntimeNode>> bt_nodes_map;
	std::unordered_map<uint32_t, std::vector<SimulatorRuntimeLink>> bt_links_map;
	uint32_t last_bt_graph_id = UINT32_MAX;
};

