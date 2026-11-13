#include "JsonFileUtility.h"

#include <filesystem>
#include <fstream>

//保存
bool JsonFileUtility::Save(const std::string& file_path, const nlohmann::json& json_data)
{
	//親ディレクトリの確認
	std::filesystem::path json_path(file_path);
	if (json_path.has_parent_path())
	{
		std::filesystem::create_directories(json_path.parent_path());
	}

	//ファイルが開けるか確認
	std::ofstream output_file(file_path);
	if (!output_file.is_open())
	{
		LOG_WARNING({ u8"保存" }, u8"保存先のJSONファイルが開けませんでした(ファイルパス: %s)", file_path.c_str());
		output_file.close();
		return false;
	}

	//JSONデータをインデント付きで書き出し
	constexpr int json_indent_space = 4;
	output_file << json_data.dump(json_indent_space);
	output_file.close();

	return true;
}

//読み込み
bool JsonFileUtility::Load(const std::string& file_path, nlohmann::json& json_data)
{
	//ファイルが開けるか確認
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		LOG_WARNING({ u8"読み込み" }, u8"読み込み先のJSONファイルが開けませんでした(ファイルパス: %s)", file_path.c_str());
		input_file.close();
		return false;
	}

	//ファイルからJSONデータへのパース
	try
	{
		input_file >> json_data;

	}
	catch (const nlohmann::json::parse_error& error)
	{
		LOG_ERROR({ u8"解析" }, u8"JSONファイルの構文解析に失敗しました(ファイルパス: %s, 詳細: %s)", file_path.c_str(), error.what());
		input_file.close();
		return false;
	}

	input_file.close();
	return true;
}
