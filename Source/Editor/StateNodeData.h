#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../StateMschine/VariableTransition.h"

class JsonSerializer;

//遷移（矢印）の編集用データ
struct TransitionNodeData
{
	std::string next_state_name;								//遷移先のステート名
	ConditionType condition_type = ConditionType::InputLength;	//判定対象
	ConditionOp operator_type = ConditionOp::Equal;				//比較演算子
	float compare_value = 0.0f;									//基準値
	std::string target_action_name;								//アクション名判定用文字列
};

//各ステートノードの編集用データ
struct StateNodeData
{
	std::string state_name;							//ステート名
	std::string animation_clip_name;				//アニメーション名
	bool is_animation_loop;							//ループ再生フラグ
	std::vector<TransitionNodeData> transitions;	//ステートから伸びている遷移のリスト

	//ノードのパラメータをJsonSerializerにバインド
	void BindToSerializer(JsonSerializer* serializer);
};