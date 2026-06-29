#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include "../Gameplay/StateMachine/StateBlackboard.h"

class StateBlackboard;
class JsonSerializer;
class AnimationComponent;

//実行時条件タイプ
enum class RuntimeConditionType
{
	Normal,		//通常比較
	Random,		//確率
	Distance,	//距離
	Ratio,		//割合
};

//比較演算子
enum class RuntimeCompareOp
{
	Equal,        //==
	NotEqual,     //!=
	Greater,      //>
	Less,         //<
	GreaterEqual, //>=
	LessEqual     //<=
};

//実行時遷移条件
struct RuntimeCondition
{
	RuntimeConditionType type;    //判定タイプ
	uint32_t hash_key;            //対象変数のハッシュキー
	float reference_value;        //判定基準値
	RuntimeCompareOp compare_op;  //比較演算子
	float param_second;           //拡張用第2パラメータ
	uint32_t secondary_hash;      //拡張用ハッシュキー
};

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

	//シリアライズ変数登録
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

private:
	//ステートマシンJSONからアニメーション対応表を抽出
	void LoadAnimationMap(StateBlackboard* blackboard);

	//ピンIDから所属するノードIDを逆引き検索
	uint32_t GetNodeIdFromPinId(uint32_t pin_id)const;

	//条件判定を実行
	bool EvaluateCondition(const RuntimeCondition& condition, StateBlackboard* blackboard) const;

private:
	std::string state_machine_path = "";	//Jsonパス
	uint32_t model_hash = 0;				//モデルのハッシュ値
	uint32_t current_node_id = UINT32_MAX;	//アクティブノード
	std::string current_animation_name = "";	//アニメーション名
	std::unordered_map<uint32_t, std::string> animation_map;	//ステートIDとアニメーション名の対応表
	std::vector<RuntimeNode> runtime_nodes;	//ノード配列
	std::vector<RuntimeLink> runtime_links;	//リンク配列
};

