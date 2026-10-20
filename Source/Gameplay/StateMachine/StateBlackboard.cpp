#include "StateBlackboard.h"

#include "Gameplay/AI/RandomJugment.h"
#include "Gameplay/AI/DistanceJugment.h"
#include "Gameplay/AI/RatioJudgment.h"
#include "Engine\Core\Input.h"

//コンストラクタ
StateBlackboard::StateBlackboard()
{

}

//デストラクタ
StateBlackboard::~StateBlackboard()
{

}

//ImGui描画
void StateBlackboard::RenderGui()
{
	//辞書の全要素をImGuiに描画
	for (auto& [hash_key, attribute] : data_map)
	{
		std::string variable_name = name_map.at(hash_key);	//の文字列
		GuiVisitor visitor(variable_name, attribute.min_value, attribute.max_value, attribute.speed);
		std::visit(visitor, attribute.value);
	}
}

//ハッシュキーからデータの値を取得
const BlackboardData& StateBlackboard::GetAttributeValue(uint32_t hash_key) const
{
	auto iterator = data_map.find(hash_key);	//指定された要素

	//要素が見つからなかった場合
	if (iterator == data_map.end())
	{
		printf("Error: StateBlackboard::GetAttributeValue - ハッシュキー [%u] のデータが存在しません。\n", hash_key);

		//先頭要素を返すと型誤認の原因になるため、未設定用のフォールバックデータを返す
		static const BlackboardData default_fallback_data = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		return default_fallback_data;
	}
	return iterator->second.value;
}

//名リストを取得
std::vector<std::string> StateBlackboard::GetRegisteredVariableNames() const
{
	std::vector<std::string>  variable_list;	//返却用のコンテナ

	for (auto iterator = allowed_variables.begin(); iterator != allowed_variables.end(); iterator++)
	{
		std::string name_string = *iterator;	//名
		variable_list.push_back(name_string);
	}

	return variable_list;
}

//ハッシュキーから名を取得
std::string StateBlackboard::GetVariableNameFromHash(uint32_t hash_key) const
{
	const std::string default_label = u8"を選択";

	//未設定か判定
	if (hash_key == 0)
	{
		return default_label;
	}

	auto iterator = name_map.find(hash_key);

	if (iterator != name_map.end())
	{
		std::string found_name = iterator->second;
		return found_name;
	}
	std::cerr << "Warning: GetVariableNameFromHash - ハッシュキー [" << hash_key << "] に対応する名が辞書に登録されていません。\n";

	std::string fallback_string = u8"Unknown(" + std::to_string(hash_key) + u8")"; 
	return fallback_string;
}

//名からハッシュキーを取得
uint32_t StateBlackboard::GetVariableHash(const std::string& variable_name) const
{
	std::string_view name_view = variable_name;	//ハッシュ計算用の文字列ビュー
	uint32_t calculated_hash = CalculateHash(name_view);

	return calculated_hash;
}

//条件を満たしているか判定
bool TransitionCondition::IsJudgment(const StateBlackboard& blackboard) const
{
	switch (type)
	{
	case ConditionNodeType::NormalCompare:
	{
		if (hash_key == 0)
		{
			return false;
		}

		if (!blackboard.HasVariable(hash_key))
		{
			printf("警告: TransitionCondition::IsJudgment - ハッシュキー [%u] の変数が登録されていないため遷移判定をスキップします。\n", hash_key);
			return false;
		}

		BlackboardData current_value = blackboard.GetAttributeValue(hash_key);
		if (std::holds_alternative<DirectX::XMFLOAT3>(current_value))
		{
			CompareVisitor vec_visitor(compart_op, vector_reference_value);
			return std::visit(vec_visitor, current_value);
		}
		CompareVisitor visitor(compart_op, reference_value);
		return std::visit(visitor, current_value);
	}
	case ConditionNodeType::Random:
	{
		const float* ref_float = std::get_if<float>(&reference_value);
		int probability = ref_float ? static_cast<int>(*ref_float) : 0;
		RandomJugment random_node(probability);
		return random_node.Check();
	}
	case ConditionNodeType::Distance:
	{
		if (hash_key == 0 || secondary_hash == 0)
		{
			return false;
		}

		BlackboardData raw_my_pos = blackboard.GetAttributeValue(hash_key);
		BlackboardData raw_target_pos = blackboard.GetAttributeValue(secondary_hash);

		const DirectX::XMFLOAT3* my_pos = std::get_if<DirectX::XMFLOAT3>(&raw_my_pos);
		const DirectX::XMFLOAT3* target_pos = std::get_if<DirectX::XMFLOAT3>(&raw_target_pos);

		if (!my_pos || !target_pos)
		{
			return false;
		}

		const float* ref_float = std::get_if<float>(&reference_value);
		float min_dist = ref_float ? *ref_float : 0.0f;

		DistanceJudgment distance_node(std::ref(*my_pos), std::ref(*target_pos), min_dist, param_second);
		return distance_node.Check();
	}
	case ConditionNodeType::Ratio:
	{
		if (hash_key == 0 || secondary_hash == 0)
		{
			return false;
		}

		BlackboardData raw_current = blackboard.GetAttributeValue(hash_key);
		BlackboardData raw_max = blackboard.GetAttributeValue(secondary_hash);

		const float* cur_val = std::get_if<float>(&raw_current);
		const float* max_val = std::get_if<float>(&raw_max);

		if (!cur_val || !max_val)
		{
			return false;
		}

		const float* ref_float = std::get_if<float>(&reference_value);
		float threshold = ref_float ? *ref_float : 0.0f;

		CompareType mapped_type = static_cast<CompareType>(compart_op);

		RatioJudgment ratio_node(std::ref(*cur_val), std::ref(*max_val), threshold, mapped_type);
		return ratio_node.Check();
	}
	case ConditionNodeType::InputCheck:
	{
		return CheckInputConditon();
	}
	}

	return false;
}

//入力判定
bool TransitionCondition::CheckInputConditon() const
{
	const int v_key_code = static_cast<int>(hash_key);
	const int input_behavior_mode = static_cast<int>(param_second);

	constexpr int key_unassigned = 0;
	if (v_key_code == key_unassigned)
	{
		return false;
	}

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
		default:
			printf("Error: TransitionCondition::CheckInputCondition - 不正な入力判定モードです: %d\n", input_behavior_mode);
			return false;
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
		default:
			printf("Error: TransitionCondition::CheckInputCondition - 不正な入力判定モードです: %d\n", input_behavior_mode);
			return false;
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
		default:
			printf("Error: TransitionCondition::CheckInputCondition - 不正な入力判定モードです: %d\n", input_behavior_mode);
			return false;
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
		default:
			printf("Error: TransitionCondition::CheckInputCondition - 不正な入力判定モードです: %d\n", input_behavior_mode);
			return false;
		}
	}
}
