#include "VariableTransition.h"
#include "StateBlackboard.h"

#include <cassert>
#include <cmath>

//コンストラクタ(数値/ビットフラグ判定)
VariableTransition::VariableTransition(const std::string& next_state_name, ConditionType type, ConditionOp op, float target_value)
	:Transition(next_state_name)
	,condition_type(type)
	,operator_type(op)
	,compare_value(target_value)
	,target_action_name("")
{

}

//コンストラクタ(特定のアクション名での判定)
VariableTransition::VariableTransition(const std::string& naxt_state_name, ConditionType type, ConditionOp op, const std::string& action_name)
	:Transition(naxt_state_name)
	,condition_type(type)
	,operator_type(op)
	,compare_value(0.0f)
	,target_action_name(action_name)
{

}

//遷移条件の判定
bool VariableTransition::IsTriggered(StateBlackboard* blackboard)
{
	assert(blackboard != nullptr && "VariableTransition::IsTriggered - Blackboard pointer is null.");
	if (!blackboard) return false;

	float current_param_value = 0.0;

	switch (condition_type)
	{
	case ConditionType::InputLength:
		current_param_value = blackboard->move_input.x + blackboard->move_input.x + blackboard->move_input.z * blackboard->move_input.z;
		return CompareValues(current_param_value, compare_value, operator_type);
		break;
	case ConditionType::ButtonCommand:
		return CheckButtonCommand(blackboard, static_cast<uint32_t>(compare_value), operator_type);
		break;
	case ConditionType::ActionPressed:
		current_param_value = blackboard->IsActionPressed(target_action_name) ? 1.0f : 0.0f;
		return CompareValues(current_param_value, compare_value, operator_type);
		break;
	case ConditionType::IsGrounded:
		current_param_value = blackboard->is_grounded ? 1.0f : 0.0f;
		return CompareValues(current_param_value, compare_value, operator_type);
		break;
	case ConditionType::TargetMoveSpeed:
		current_param_value = blackboard->target_move_speed;
		return CompareValues(current_param_value, compare_value, operator_type);
		break;
	default:
		assert(false && "VariableTransition::IsTriggered - Unsupported ConditionType.");
		break;
	}

	return false;
}

//数値の比較演算
bool VariableTransition::CompareValues(float current_value, float target_value, ConditionOp op)
{
	constexpr float EPSILON = 0.001f;

	switch (op)
	{
	case ConditionOp::Equal:		return std::fabsf(current_value - target_value) < EPSILON;
	case ConditionOp::NotEqual:		return std::fabsf(current_value - target_value) >= EPSILON;
	case ConditionOp::GreaterThan:	return current_value > (target_value + EPSILON);
	case ConditionOp::LessThen:		return current_value < (target_value - EPSILON);
	case ConditionOp::GreaterEqual:	return current_value >= (target_value - EPSILON);
	case ConditionOp::LessEqual:	return current_value <= (target_value + EPSILON);
	default:assert(false && "VariableTransition::CompareValues - Unknown ConditionOp."); break;
	}
	return false;
}

//ボタンの組み合わせビット演算
bool VariableTransition::CheckButtonCommand(StateBlackboard* blackboard, uint32_t target_mask, ConditionOp op)
{
	uint32_t current_input_flags = blackboard->current_input_flags;

	switch (op)
	{
	case ConditionOp::Equal:
		return current_input_flags == target_mask;
		break;
	case ConditionOp::NotEqual:
		return current_input_flags != target_mask;
		break;
	case ConditionOp::GreaterEqual:
		return (current_input_flags & target_mask) == target_mask;
		break;
	default:
		assert(false && "VariableTransition::CheckButtonCommand - Unsupported ConditionOp.");
		break;
	}
	return false;
}
