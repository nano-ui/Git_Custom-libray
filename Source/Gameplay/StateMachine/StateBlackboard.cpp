#include "StateBlackboard.h"

#include "Gameplay/AI/RandomJugment.h"
#include "Gameplay/AI/DistanceJugment.h"
#include "Gameplay/AI/RatioJudgment.h"

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
		std::cerr << "Error : StateBlackboard::GetAttributeValue - 指定されたハッシュキー [" << hash_key << "] のデータは存在しません。\n";

		if (data_map.empty())
		{
			static const BlackboardData default_fallback_data = 0.0f;
			return default_fallback_data;
		}

		return data_map.begin()->second.value;
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
	//判定ノードの識別
	switch (type)
	{
	case ConditionNodeType::NormalCompare:
	{
		if (hash_key == 0)
		{
			return false;
		}

		BlackboardData current_value = blackboard.GetAttributeValue(hash_key);	//比較する値
		CompareVisitor visitor(compart_op, reference_value);
		return std::visit(visitor, current_value);
	}
	case ConditionNodeType::Random:
	{
		const float* ref_float = std::get_if<float>(&reference_value);
		int probability = ref_float ? static_cast<int>(*ref_float) : 0;	//確率値
		RandomJugment random_node(probability);
		return random_node.Check();
	}
	case ConditionNodeType::Distance:
	{
		if (hash_key == 0 || secondary_hash == 0)
		{
			return false;
		}

		BlackboardData raw_my_pos = blackboard.GetAttributeValue(hash_key);	//自身の座標
		BlackboardData raw_target_pos = blackboard.GetAttributeValue(secondary_hash);	//対象の座標

		const DirectX::XMFLOAT3* my_pos = std::get_if<DirectX::XMFLOAT3>(&raw_my_pos);	//自身の座標
		const DirectX::XMFLOAT3* target_pos = std::get_if<DirectX::XMFLOAT3>(&raw_target_pos);	//対象の座標

		if (!my_pos || !target_pos)
		{
			return false;
		}

		const float* ref_float = std::get_if<float>(&reference_value);	//最小距離
		float min_dist = ref_float ? *ref_float : 0.0f;	//確定した最小距離

		DistanceJudgment distance_node(std::ref(*my_pos), std::ref(*target_pos), min_dist, param_second);
		return distance_node.Check();
	}
	case ConditionNodeType::Ratio:
	{
		if (hash_key == 0 || secondary_hash == 0)
		{
			return false;
		}

		BlackboardData raw_current = blackboard.GetAttributeValue(hash_key);	//読み込んだ現在値
		BlackboardData raw_max = blackboard.GetAttributeValue(secondary_hash);	//読み込んだ最大値

		const float* cur_val = std::get_if<float>(&raw_current);	//確定した現在値
		const float* max_val = std::get_if<float>(&raw_max);		//確定した最大値

		if (!cur_val || !max_val)
		{
			return false;
		}

		const float* ref_float = std::get_if<float>(&reference_value);	//読み込んだ基準値
		float threshold = ref_float ? *ref_float : 0.0f;				//確定した基準値

		CompareType mapped_type = static_cast<CompareType>(compart_op);	//比較演算子

		RatioJudgment ratio_node(std::ref(*cur_val), std::ref(*max_val), threshold, mapped_type);
		return ratio_node.Check();
	}
	}

	return false;
}
