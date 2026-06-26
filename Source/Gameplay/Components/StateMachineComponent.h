#pragma once

#include <string>
#include <cstdint>

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
	void Initialize(const std::string& default_path);

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

private:
	std::string state_machine_path = "";	//Jsonパス
	uint32_t model_hash = 0;				//モデルのハッシュ値
};

