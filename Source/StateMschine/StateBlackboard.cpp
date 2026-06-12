#include "StateBlackboard.h"
#include "VariableTransition.h"

//コンストラクタ
StateBlackboard::StateBlackboard()
{
	move_input = { 0.0f,0.0f,0.0f };
	current_input_flags = 0;

	action_category = ActionCategory::Idle;
	sub_action_id = 0;
	target_move_speed = 0.0f;

	is_grounded = false;
	current_velocity = { 0.0f,0.0f,0.0f };

	generic_parameters.clear();
}

//デストラクタ
StateBlackboard::~StateBlackboard()
{
	generic_parameters.clear();
}

//汎用アクセス関数
bool StateBlackboard::IsActionPressed(const std::string& action_name) const
{
	auto iterator = action_states.find(action_name);
	if (iterator == action_states.end())
	{
		return false;
	}

	return iterator->second;
}

//入力状態を書き込む
void StateBlackboard::SetActionState(const std::string& action_name, bool is_pressed)
{
	action_states[action_name] = is_pressed;
}
