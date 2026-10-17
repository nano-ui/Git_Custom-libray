#include "JsonSerializer.h"
#include <fstream>
#include <Windows.h>

//コンストラクタ
JsonSerializer::JsonSerializer()
{
}

//デストラクタ
JsonSerializer::~JsonSerializer()
{
	registered_properties.clear();
}

//JSON形式でデータをファイルへ書き出す
void JsonSerializer::SaveToFile(const std::string& file_path, SerializeTarget target) 
{
	nlohmann::json root_json;
	SaveToObject(root_json, target);

	std::ofstream output_file(file_path);
	if (output_file.is_open())
	{
		constexpr int json_indent_space = 4;
		output_file << root_json.dump(json_indent_space);
		output_file.close();
	}
	else
	{
		OutputDebugStringA("[JsonSerializer エラー] SaveToFile: 保存先ファイルが開けませんでした。\n");
	}
}

//JSONファイルからデータを群へ読み込む
bool JsonSerializer::LoadFromFile(const std::string& file_path, SerializeTarget target)
{
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		return false;
	}

	nlohmann::json root_json;
	input_file >> root_json;
	input_file.close();

	LoadFromObject(root_json, target);
	return true;
}

//JSONオブジェクトへ登録データを直接書き出す
void JsonSerializer::SaveToObject(nlohmann::json& root_json, SerializeTarget target) 
{
	//登録されているすべてのプロパティをループ処理してJSONノードへ変換
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];	//要素の参照

		if ((current_data.target & target) != static_cast<SerializeTarget>(0))
		{
			if (current_data.property_interface != nullptr)
			{
				current_data.property_interface->SaveTo(root_json, current_data.name);
			}
			else
			{
				OutputDebugStringA("[JsonSerializer 警告] SaveToObject: property_interface が nullptr です。\n");
			}
		}
	}
}

//JSONオブジェクトからデータを直接読み込む
void JsonSerializer::LoadFromObject(const nlohmann::json& root_json, SerializeTarget target) 
{
	//登録されているすべてのプロパティをループ処理して値を復元
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];	//要素の参照

		if ((current_data.target & target) != static_cast<SerializeTarget>(0))
		{
			if (current_data.property_interface != nullptr)
			{
				current_data.property_interface->LoadFrom(root_json, current_data.name);
			}
			else
			{
				OutputDebugStringA("[JsonSerializer 警告] LoadFromObject: property_interface が nullptr です。\n");
			}
		}
	}
}