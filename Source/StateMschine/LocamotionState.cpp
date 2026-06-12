#include "LocamotionState.h"
#include "StateBlackboard.h"
#include <cmath>
#include <cassert>

//コンストラクタ
LocamotionState::LocamotionState()
	:State("Locomotion")
	, move_speed_max(5.0f)
{
}

//デストラクタ
LocamotionState::~LocamotionState()
{

}

//状態に入った瞬間
void LocamotionState::Enter()
{

}

//更新
void LocamotionState::Update(float elapsed_time, StateBlackboard* blackboard)
{
	assert(blackboard != nullptr && "LocomotionState::Update - Blackboard pointer is null.");
	if (!blackboard) return;

	DirectX::XMFLOAT3 input = blackboard->move_input;
	float input_lenght = CalculateINputLength(input);
	blackboard->action_category = ActionCategory::Locomotion;
	blackboard->target_move_speed = input_lenght * move_speed_max;

}

//状態から抜ける瞬間
void LocamotionState::Exit()
{
}

//入力ベクトルの長さを計算
float LocamotionState::CalculateINputLength(const DirectX::XMFLOAT3& input)
{
	float lenght = std::sqrtf(input.x * input.x + input.z * input.z);

	if (lenght > 1.0f)
	{
		lenght = 1.0f;
	}

	return lenght;
}
