#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include "Gameplay\StateMachine\StateBlackboard.h"

class StateBlackboard;
class JsonSerializer;
class AnimationComponent;

//実行時リンク
struct RuntimeLink
{
	uint32_t id;                                //接続線の固有ID
	uint32_t start_pin_id;                      //開始ピンのID
	uint32_t end_pin_id;                        //終了ピンのID
	std::vector<TransitionCondition> conditions;   //このリンクに属する遷移条件配列
};

//実行時ノード
struct RuntimeNode
{
	uint32_t id;					//ノードの固有ID
	std::string name;				//ステートの表示名
	std::string animation_name;		//再生するアニメーション名
	std::vector<uint32_t> inputs;	//入力ピンのID配列
	std::vector<uint32_t> outputs;	//出力ピンのID配列
	bool is_sub_graph = false;		//サブグラフノードのフラグ
	uint32_t sub_graph_id = 0;		//紐づく位階階層のグラフID
	bool is_loop;					//ループ再生フラグ
	bool is_root_motion = false;	//ルートモーション有効フラグ
	uint32_t parent_node_id;		//親サブグラフノードのID
};


class StateMachineComponent
{
public:
	//コンストラクタ
	StateMachineComponent();

	//デストラクタ
	~StateMachineComponent();

	//初期化
	void Initialize(StateBlackboard* blackboard);

	//更新
	void Update(float elapsed_time, StateBlackboard* blackboard);

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

	//アニメーションループを取得
	bool GetAnimationLoop()const { return current_animation_loop; }

	//現在のステートがルートモーションを有効にしているか取得
	bool IsCurrentRootMotionEnbled()const;

	//リフレッシュ要求
	void RequestReload() { is_pending_reload = true; }

private:
	//ステートマシンJSONからアニメーション対応表を抽出
	void LoadAnimationMap(StateBlackboard* blackboard);

	//ピンIDから所属するノードIDを逆引き検索
	uint32_t GetNodeIdFromPinId(uint32_t pin_id)const;

	//指定されたノードIDの親サブグラフノードIDを逆引き
	uint32_t GetParentNodeId(uint32_t node_id)const;

private:
	std::string state_machine_path = "";	//Jsonパス
	uint32_t model_hash = 0;				//モデルのハッシュ値
	uint32_t current_node_id = UINT32_MAX;	//アクティブノード
	std::string current_animation_name = "";	//アニメーション名
	bool current_animation_loop;				//アニメーション再生フラグ
	bool is_pending_reload = false;				//リロード予約フラグ
	std::unordered_map<uint32_t, std::string> animation_map;	//ステートIDとアニメーション名の対応表
	std::vector<RuntimeNode> runtime_nodes;	//ノード配列
	std::vector<RuntimeLink> runtime_links;	//リンク配列
	std::unordered_map<uint32_t, uint32_t> layer_entry_nodes;	//各階層ごとの先頭エントリーIDマップ
};

