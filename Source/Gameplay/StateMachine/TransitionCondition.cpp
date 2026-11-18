#include "TransitionCondition.h"

#include <cmath>
#include <functional>
#include <winuser.h>

#include "Gameplay\Common\Blackboard.h"
#include "Gameplay\AI\RandomJugment.h"
#include "Gameplay\AI\DistanceJugment.h"
#include "Gameplay\AI\RatioJudgment.h"
#include "Engine\Core\Input.h"
#include "Engine\Core\Log\Log.h"

//条件を満たしているか判定
bool TransitionCondition::IsJudgment(const Blackboard& blackboard) const
{
	//判定タイプに応じて各評価処理へ振り分け
	switch (type)
	{
	case ConditionNodeType::NormalCompare:
		return EvaluateNormalCompare(blackboard);

	case ConditionNodeType::Random:
	{
		const int probability = static_cast<int>(reference_value);
		RandomJugment random_node(probability);
		return random_node.Check();
	}

	case ConditionNodeType::Distance:
		return EvaluateDistance(blackboard);

	case ConditionNodeType::Ratio:
		return EvaluateRatio(blackboard);

	case ConditionNodeType::InputCheck:
		return CheckInputConditon();

	default:
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"未対応の条件判定タイプが指定されました: %d", static_cast<int>(type));
		return false;
	}
}

//通常比較（数値・フラグ・ベクトル）の判定
bool TransitionCondition::EvaluateNormalCompare(const Blackboard& blackboard) const
{
	//1. 変数名の有効性チェックと検索キーの初回キャッシュ
	if (variable_name.empty())
	{
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"比較対象の変数名が空のため判定をスキップします");
		return false;
	}

	if (cached_hash == INVALID_HASH_KEY)
	{
		cached_hash = blackboard.GetKeyId(variable_name);
	}

	//2. 登録されている型を判別して値を取得し、共通のfloat比較へ流し込む
	float float_val = 0.0f;
	if (blackboard.TryGetValue(cached_hash, float_val))
	{
		return CompareFloat(float_val, reference_value);
	}

	int int_val = 0;
	if (blackboard.TryGetValue(cached_hash, int_val))
	{
		return CompareFloat(static_cast<float>(int_val), reference_value);
	}

	bool bool_val = false;
	if (blackboard.TryGetValue(cached_hash, bool_val))
	{
		constexpr float true_as_float = 1.0f;
		constexpr float false_as_float = 0.0f;
		const float converted_val = bool_val ? true_as_float : false_as_float;
		return CompareFloat(converted_val, reference_value);
	}

	DirectX::XMFLOAT3 vec_val = { 0.0f, 0.0f, 0.0f };
	if (blackboard.TryGetValue(cached_hash, vec_val))
	{
		return CompareFloat(vec_val.x, vector_reference_value.x) &&
			CompareFloat(vec_val.y, vector_reference_value.y) &&
			CompareFloat(vec_val.z, vector_reference_value.z);
	}

	//3. 未登録または非対応の型だった場合の安全処理
	LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"変数 [%s] がブラックボードに未登録、または非対応の型のため判定をスキップします", variable_name.c_str());
	return false;
}

//2点間の距離条件の判定
bool TransitionCondition::EvaluateDistance(const Blackboard& blackboard) const
{
	//1. 自身と対象の変数名チェックおよび検索キーのキャッシュ
	if (variable_name.empty() || secondary_variable_name.empty())
	{
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"距離判定の座標変数名が未設定のためスキップします");
		return false;
	}

	if (cached_hash == INVALID_HASH_KEY)
	{
		cached_hash = blackboard.GetKeyId(variable_name);
	}
	if (cached_secondary_hash == INVALID_HASH_KEY)
	{
		cached_secondary_hash = blackboard.GetKeyId(secondary_variable_name);
	}

	//2. ブラックボードから2点の3D座標を取得
	DirectX::XMFLOAT3 my_pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 target_pos = { 0.0f, 0.0f, 0.0f };

	if (!blackboard.TryGetValue(cached_hash, my_pos) || !blackboard.TryGetValue(cached_secondary_hash, target_pos))
	{
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"距離判定用の座標変数 [%s] または [%s] の取得に失敗しました", variable_name.c_str(), secondary_variable_name.c_str());
		return false;
	}

	//3. 距離判定ノードによる範囲チェックの実行
	DistanceJudgment distance_node(std::ref(my_pos), std::ref(target_pos), reference_value, param_second);
	return distance_node.Check();
}

//現在値と最大値の割合条件の判定
bool TransitionCondition::EvaluateRatio(const Blackboard& blackboard) const
{
	//1. 現在値と最大値の変数名チェックおよび検索キーのキャッシュ
	if (variable_name.empty() || secondary_variable_name.empty())
	{
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"割合判定の変数名が未設定のためスキップします");
		return false;
	}

	if (cached_hash == INVALID_HASH_KEY)
	{
		cached_hash = blackboard.GetKeyId(variable_name);
	}
	if (cached_secondary_hash == INVALID_HASH_KEY)
	{
		cached_secondary_hash = blackboard.GetKeyId(secondary_variable_name);
	}

	//2. ブラックボードから現在値と最大値を取得
	float current_val = 0.0f;
	float max_val = 0.0f;

	if (!blackboard.TryGetValue(cached_hash, current_val) || !blackboard.TryGetValue(cached_secondary_hash, max_val))
	{
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"割合判定用の変数 [%s] または [%s] の取得に失敗しました", variable_name.c_str(), secondary_variable_name.c_str());
		return false;
	}

	//3. 割合判定ノードによる閾値チェックの実行
	const CompareType mapped_type = static_cast<CompareType>(compart_op);
	RatioJudgment ratio_node(std::ref(current_val), std::ref(max_val), reference_value, mapped_type);
	return ratio_node.Check();
}

//2つのfloat値を指定の演算子で比較
bool TransitionCondition::CompareFloat(float value, float reference) const
{
	constexpr float float_epsilon = 0.0001f;

	//指定された比較演算子に応じて誤差を考慮した数値比較を実行
	switch (compart_op)
	{
	case CompareOperator::Equal:        return std::abs(value - reference) < float_epsilon;
	case CompareOperator::NotEqual:     return std::abs(value - reference) >= float_epsilon;
	case CompareOperator::Greater:      return value > reference;
	case CompareOperator::Less:         return value < reference;
	case CompareOperator::GreaterEqual: return value >= reference;
	case CompareOperator::LessEqual:    return value <= reference;
	default:
		LOG_WARNING({ u8"ステートマシン", u8"遷移判定" }, u8"未定義の比較演算子が指定されました: %d", static_cast<int>(compart_op));
		return false;
	}
}

//キーボード・マウスの入力条件の判定
bool TransitionCondition::CheckInputConditon() const
{
	//1. 変数名文字列から仮想キーコードへの変換と未設定チェック
	if (variable_name.empty())
	{
		return false;
	}

	int v_key_code = 0;
	try
	{
		v_key_code = std::stoi(variable_name);
	}
	catch (...)
	{
		LOG_WARNING({ u8"ステートマシン", u8"入力判定" }, u8"キーコードの変換に失敗しました: %s", variable_name.c_str());
		return false;
	}

	constexpr int key_unassigned = 0;
	if (v_key_code == key_unassigned)
	{
		return false;
	}

	//2. 入力モード（押下中・瞬間・未押下・離上）に応じた入力状態の判定
	const int input_behavior_mode = static_cast<int>(param_second);
	constexpr int mode_press = 0;
	constexpr int mode_trigger = 1;
	constexpr int mode_not_press = 2;
	constexpr int mode_release = 3;

	if (v_key_code == VK_LBUTTON)
	{
		switch (input_behavior_mode)
		{
		case mode_press:     return Input::Instance().IsMousePress(Input::mouse_left);
		case mode_trigger:   return Input::Instance().IsMouseTrigger(Input::mouse_left);
		case mode_not_press: return !Input::Instance().IsMousePress(Input::mouse_left);
		case mode_release:   return Input::Instance().IsMouseRelease(Input::mouse_left);
		default:             return false;
		}
	}
	else if (v_key_code == VK_RBUTTON)
	{
		switch (input_behavior_mode)
		{
		case mode_press:     return Input::Instance().IsMousePress(Input::mouse_right);
		case mode_trigger:   return Input::Instance().IsMouseTrigger(Input::mouse_right);
		case mode_not_press: return !Input::Instance().IsMousePress(Input::mouse_right);
		case mode_release:   return Input::Instance().IsMouseRelease(Input::mouse_right);
		default:             return false;
		}
	}
	else if (v_key_code == VK_MBUTTON)
	{
		switch (input_behavior_mode)
		{
		case mode_press:     return Input::Instance().IsMousePress(Input::mouse_middle);
		case mode_trigger:   return Input::Instance().IsMouseTrigger(Input::mouse_middle);
		case mode_not_press: return !Input::Instance().IsMousePress(Input::mouse_middle);
		case mode_release:   return Input::Instance().IsMouseRelease(Input::mouse_middle);
		default:             return false;
		}
	}
	else
	{
		switch (input_behavior_mode)
		{
		case mode_press:     return Input::Instance().IsKeyPress(v_key_code);
		case mode_trigger:   return Input::Instance().IsKeyTrigger(v_key_code);
		case mode_not_press: return !Input::Instance().IsKeyPress(v_key_code);
		case mode_release:   return Input::Instance().IsKeyRelease(v_key_code);
		default:             return false;
		}
	}
}