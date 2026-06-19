#include "StateBlackboard.h"

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
		std::string variable_name = name_map.at(hash_key);	//変数の文字列
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
		std::cerr << "Error : 指定されたハッシュキー [" << hash_key << "] の変数は存在しません。\n";
		return data_map.begin()->second.value;
	}
	return iterator->second.value;
}

//変数名リストを取得
std::vector<std::string> StateBlackboard::GetRegisteredVariableNames() const
{
	std::vector<std::string>  variable_list;	//返却用のコンテナ

	for (auto iterator = allowed_variables.begin(); iterator != allowed_variables.end(); iterator++)
	{
		std::string name_string = *iterator;	//変数名
		variable_list.push_back(name_string);
	}

	return variable_list;
}

//ハッシュキーから変数名を取得
std::string StateBlackboard::GetVariableNameFromHash(uint32_t hash_key) const
{
	const std::string default_label = u8"変数を選択";

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
	std::cerr << "Warning: GetVariableNameFromHash - ハッシュキー [" << hash_key << "] に対応する変数名が辞書に登録されていません。\n";

	std::string fallback_string = u8"Unknown(" + std::to_string(hash_key) + u8")"; 
	return fallback_string;
}

//変数名からハッシュキーを取得
uint32_t StateBlackboard::GetVariableHash(const std::string& variable_name) const
{
	std::string_view name_view = variable_name;	//ハッシュ計算用の文字列ビュー
	uint32_t calculated_hash = CalculateHash(name_view);

	return calculated_hash;
}

//条件を満たしているか判定
bool TransitionCondition::IsJudgment(const StateBlackboard& blackboard) const
{
	BlackboardData current_value = blackboard.GetAttributeValue(hash_key);	//比較する値
	CompareVisitor visitor(compart_op, reference_value);
	return std::visit(visitor, current_value);
}
