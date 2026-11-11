#pragma once

#include <string>
#include "ThiedParty\json.hpp"

class JsonFileUtility
{
public:
	//ï€ë∂
	static bool Save(const std::string& file_path, const nlohmann::json& json_data);

	//ì«Ç›çûÇ›
	static bool Load(const std::string& file_path, nlohmann::json& json_data);
};

