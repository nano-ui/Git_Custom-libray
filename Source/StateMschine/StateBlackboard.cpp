#include "StateBlackboard.h"

//コンストラクタ
StateBlackboard::StateBlackboard()
{
	move_input = { 0.0f,0.0f,0.0f };
	is_jump_pressed = false;
	is_attack_pressed = false;
	is_avoid_pressed = false;

	action_category = ActionCategory::Idle;
	sub_action_id = 0;
	target_move_speed = 0.0f;

	is_grounded = false;
	current_velocity = { 0.0f,0.0f,0.0f };
}

//デストラクタ
StateBlackboard::~StateBlackboard()
{

}
