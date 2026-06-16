#pragma once

#include <string>
#include <vector>
#include <memory>
#include <imgui.h>

#include "../StateMschine/VariableTransition.h"

class JsonSerializer;

//適用する計算方法
enum class ParameterOp :int
{
	Assign = 0,		//代入(=)
	Add,			//加算(+)
	Subtract,		//減算(-)
	Multiply,		//乗算(*)
	Devide,			//除算(/)
};

//エディタで設定する1つ分の変数変更モディファイア
struct ParameterModifierData
{
	std::string target_variable_name;					//登録名	
	ParameterOp operator_type = ParameterOp::Assign;	//計算方法
	float value = 0.0f;									//適用する値
};

enum class TransitionBlendMode : int
{
	CombineWithCommon,	//共通条件を組み合わせる
	CverrideCommon		//個別条件のみを適用
};

//個々の条件を定義
struct ConditionData
{
	ConditionType condition_type = ConditionType::InputLength;	// 判定対象
	ConditionOp operator_type = ConditionOp::Equal;				// 比較演算子
	float compare_value = 0.0f;									// 基準値
	std::string target_action_name;								// アクション名判定用文字列
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
	std::vector<ConditionData> conditions;			//複数の条件を管理
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
	std::vector<ConditionData> common_conditions;			//ステート共通の出力遷移条件を複数保持・管理
	std::vector<ParameterModifierData> parameter_modifiers;	//ステート中に実行する変数の書き換えリスト

	//ノードのパラメータをJsonSerializerにバインド
	void BindToSerializer(JsonSerializer* serializer);
};