#pragma once

#include <DirectXMath.h>

#include "State.h"

class StateBlackboard;

class IdleState : public State
{
public:
	//コンストラクタ
	IdleState();

	//デストラクタ
	~IdleState();

	//状態に入った瞬間
	void Enter()override;

	//更新
	void Update(float elapsed_time, StateBlackboard* blackboard)override;

	//状態から抜ける瞬間
	void Exit()override;
};

