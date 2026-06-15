#pragma once

#include <string>
#include <vector>
#include <memory>
#include <imgui.h>

#include "../StateMschine/VariableTransition.h"

class JsonSerializer;

enum class TransitionBlendMode : int
{
	CombineWithCommon,	//共通条件を組み合わせる
	CverrideCommon		//個別条件のみを適用
};

//遷移（矢印）の編集用データ
struct TransitionNodeData
{
	std::string next_state_name;								//遷移先のステート名
	ConditionType condition_type = ConditionType::InputLength;	//判定対象
	ConditionOp operator_type = ConditionOp::Equal;				//比較演算子
	float compare_value = 0.0f;									//基準値
	std::string target_action_name;								//アクション名判定用文字列
	TransitionBlendMode blend_mode = TransitionBlendMode::CombineWithCommon;	//共通条件とのブレンド設定
};

//各ステートノードの編集用データ
struct StateNodeData
{
	std::string state_name;							//ステート名
	std::string animation_clip_name;				//アニメーション名
	bool is_animation_loop;							//ループ再生フラグ
	std::vector<TransitionNodeData> transitions;	//ステートから伸びている遷移のリスト

	ImVec2 graph_position = ImVec2(100.0f, 100.0f); //グラフ画面上におけるノードの配置座標
	bool is_dragging = false;                       //現在マウスでドラッグ移動中かどうかのフラグ

	bool has_common_condition = false;	//共通条件を使用するかどうかのフラグ
	ConditionType common_condition_type = ConditionType::IsGrounded;
	ConditionOp common_operator_type = ConditionOp::Equal;
	float common_compare_value = 1.0f;
	std::string common_target_action_name;

	//ノードのパラメータをJsonSerializerにバインド
	void BindToSerializer(JsonSerializer* serializer);
};