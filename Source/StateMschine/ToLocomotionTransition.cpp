#include "ToLocomotionTransition.h"
#include "StateBlackboard.h"

#include <cassert>

//コンストラクタ
ToLocomotionTransition::ToLocomotionTransition()
	:Transition("Locomotion")
{

}

//遷移条件の判定
bool ToLocomotionTransition::IsTriggered(StateBlackboard* blackboard)
{
	assert(blackboard != nullptr && "ToLocomotionTransition::IsTriggered - Blackboard pointer is null.");
	if (!blackboard) return false;

	DirectX::XMFLOAT3 input = blackboard->move_input;
	float input_length_sq = input.x * input.x + input.z * input.z;
	constexpr float EPSILON = 0.001;

	if (input_length_sq > EPSILON)
	{
		return true;
	}

	return false;
}
