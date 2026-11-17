#include "Blackboard.h"

//コンストラクタ
Blackboard::Blackboard()
{
	data_map.clear();
	name_map.clear();
}

//親ブラックボードの設定
void Blackboard::SetParent(std::shared_ptr<Blackboard> blackboard)
{
	parent_blackboard = blackboard;
}

//変数名リストの取得
std::vector<std::string> Blackboard::GetRegisteredNames() const
{
	std::vector<std::string> variable_list = {};
	variable_list.reserve(name_map.size());

	for (auto variable = name_map.begin(); variable != name_map.end(); ++variable)
	{
		variable_list.push_back(variable->second);
	}
	return variable_list;
}