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

//条件を満たしているか判定
bool TransitionCondition::IsJudgment(const StateBlackboard& blackboard) const
{
	BlackboardData current_value = blackboard.GetAttributeValue(hash_key);	//比較する値
	CompareVisitor visitor(compart_op, reference_value);
	return std::visit(visitor, current_value);
}
