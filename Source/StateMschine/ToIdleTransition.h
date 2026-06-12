#pragma once
#include "State.h"

class ToIdleTransition : public Transition
{
public:
	//コンストラクタ
	ToIdleTransition();

	//デストラクタ
	~ToIdleTransition()override = default;

	//遷移条件の判定
	bool IsTriggered(StateBlackboard* blackboard)override;
};

