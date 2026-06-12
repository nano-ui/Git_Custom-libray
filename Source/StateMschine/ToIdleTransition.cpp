#include "ToIdleTransition.h"
#include "StateBlackboard.h"

#include <cassert>

//コンストラクタ
ToIdleTransition::ToIdleTransition()
	:Transition("Idle")
{

}

//遷移条件の判定
bool ToIdleTransition::IsTriggered(StateBlackboard* blackboard)
{
	assert(blackboard != nullptr && "ToIdleTransition::IsTriggered - Blackboard pointer is null.");
	if (!blackboard) return false;

	DirectX::XMFLOAT3 input = blackboard->move_input;
	float input_length_sq = input.x * input.x + input.z * input.z;

	constexpr float EPSILON = 0.001f;
	if (input_length_sq <= EPSILON)
	{
		return true;
	}

	return false;
}
