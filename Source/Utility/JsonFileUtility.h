#pragma once

#include <string>
#include "ThiedParty\json.hpp"
#include "Engine\Core\Log\Log.h"

class JsonFileUtility
{
public:
	//保存
	static bool Save(const std::string& file_path, const nlohmann::json& json_data);

	//読み込み
	static bool Load(const std::string& file_path, nlohmann::json& json_data);

	//JSONファイルの型チェックと値の取得
	template<typename T>
	static bool TryGetValue(const nlohmann::json& json_data, const std::string& key, T& out_value)
	{
		if (!json_data.contains(key))return false;
		try
		{
			out_value = json_data[key].get<T>();
			return true;
		}
		catch (const nlohmann::json::type_error& error)
		{
			LOG_WARNING({ u8"復元" }, u8"型が一致しません(詳細:%s)", error.what());
			return false;
		}
	}
};

