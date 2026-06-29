#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>

class StateBlackboard;
class JsonSerializer;

class StateMachineComponent
{
public:
	//コンストラクタ
	StateMachineComponent();

	//デストラクタ
	~StateMachineComponent();

	//初期化
	void Initialize();

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

private:
	//ステートマシンJSONからアニメーション対応表を抽出
	void LoadAnimationMap();

private:
	std::string state_machine_path = "";	//Jsonパス
	uint32_t model_hash = 0;				//モデルのハッシュ値
	std::unordered_map<uint32_t, std::string> animation_map;	//ステートIDとアニメーション名の対応表
};

