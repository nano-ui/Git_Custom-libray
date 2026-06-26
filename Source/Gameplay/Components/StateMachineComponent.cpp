#include "StateMachineComponent.h"
#include "../Serialization/JsonSerializer.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"

//コンストラクタ
StateMachineComponent::StateMachineComponent()
{
	state_machine_path = "";
	model_hash = 0;
}

//デストラクタ
StateMachineComponent::~StateMachineComponent() = default;

//初期化
void StateMachineComponent::Initialize(const std::string& default_path)
{
	//パスが空か判定
	if (state_machine_path.empty())
	{
		state_machine_path = default_path;
	}
}

//更新
void StateMachineComponent::Update(float elapsed_time, StateBlackboard* blackboard)
{
	//ブラックボードが有効か確認
	if (!blackboard)
	{
		return;
	}
}

//シリアライズ変数登録
void StateMachineComponent::SetupSerialization(JsonSerializer* serializer)
{
	//シリアライザのポインタが有効か確認
	if (serializer)
	{
		serializer->RegisterVariable("StateMachinePath", &state_machine_path);
	}
}
