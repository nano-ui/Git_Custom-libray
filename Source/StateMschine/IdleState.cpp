#include "IdleState.h"
#include "StateBlackboard.h"

#include <cassert>

//コンストラクタ
IdleState::IdleState()
	:State("Idle")
{

}

//デストラクタ
IdleState::~IdleState()
{

}

//状態に入った瞬間
void IdleState::Enter()
{

}

//更新
void IdleState::Update(float elapsed_time, StateBlackboard* blackboard)
{
	assert(!blackboard && "IdleState::Update - Blackboard pointer is null.");
	if (!blackboard)return;

	//指示をブラックボードに書き込む
	blackboard->action_category = ActionCategory::Idle;
	blackboard->target_move_speed = 0.0f;
}

//状態から抜ける瞬間
void IdleState::Exit()
{

}

