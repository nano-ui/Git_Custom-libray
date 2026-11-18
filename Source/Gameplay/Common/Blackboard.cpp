#include "Blackboard.h"

//•Ï”–¼ƒŠƒXƒg‚Ìæ“¾
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