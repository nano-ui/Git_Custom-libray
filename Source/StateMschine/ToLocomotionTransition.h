#pragma once
#include "State.h"


class ToLocomotionTransition : public Transition
{
public:
	//コンストラクタ
	ToLocomotionTransition();

	//デストラクタ
	~ToLocomotionTransition()override = default;

	//遷移条件の判定
	bool IsTriggered(StateBlackboard* blackboard)override;
};

