#pragma once

#include "State.h"

//遷移条件の判定対象
enum class ConditionType
{
	InputLength,		//移動入力ベクトルの長さ
	ButtonCommand,		//ボタン単体、ボタン組み合わせのビット判定
	ActionPressed,		//アクション名が押されているか判定
	IsGrounded,			//接地フラグの状態
	TargetMoveSpeed,	//目標移動速度の数値
};

//比較演算子の種類
enum class ConditionOp
{
	Equal,			//イコール
	NotEqual,		//ノットイコール
	GreaterThan,	//より大きい
	LessThen,		//未満
	GreaterEqual,	//以上
	LessEqual,		//以下
};

class VariableTransition : public Transition
{
public:
	//コンストラクタ(数値/ビットフラグ判定)
	VariableTransition(const std::string& next_state_name, ConditionType type, ConditionOp op, float target_value);

	//コンストラクタ(特定のアクション名での判定)
	VariableTransition(const std::string& naxt_state_name, ConditionType type, ConditionOp op, const std::string& action_name);

	//デストラクタ
	~VariableTransition()override = default;

	//遷移条件の判定
	bool IsTriggered(StateBlackboard* blackboard)override;

private:
	bool CompareValues(float current_value, float target_value, ConditionOp op);

	//ボタンの組み合わせビット演算
	bool CheckButtonCommand(StateBlackboard* blackboard, uint32_t target_mask, ConditionOp op);

private:
	ConditionType condition_type;	//比較対象のタイプ
	ConditionOp operator_type;		//比較演算子
	float compare_value;			//比較する基準値
	std::string target_action_name;	//対象とするアクション名
};

