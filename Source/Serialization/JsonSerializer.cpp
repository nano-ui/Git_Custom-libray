#include "JsonSerializer.h"
#include <fstream>

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
void JsonSerializer::SaveToFile(const std::string& file_path)
{
	nlohmann::json root_json;	//JSONデータのルート階層オブジェクト

	//登録変数の一括セーブパース
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];	//要素の参照
		
		if (current_data.property_interface != nullptr)
		{
			current_data.property_interface->SaveTo(root_json, current_data.name);
		}
	}

	//テキストファイルへの書き出し
	std::ofstream output_file(file_path);	
	if (output_file.is_open())
	{
		constexpr int json_indent_space = 4;	//インデント幅
		output_file << root_json.dump(json_indent_space);
		output_file.close();
	}
}

//JSONファイルからデータを変数群へ読み込む
bool JsonSerializer::LoadFromFile(const std::string& file_path)
{
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		return false;
	}

	nlohmann::json root_json;	//パース結果を受け取るJSONオブジェクト
	input_file >> root_json;
	input_file.close();

	//JSONデータから各登録変数への自動復元
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];	//要素の参照

		if (current_data.property_interface != nullptr)
		{
			current_data.property_interface->LoadFrom(root_json, current_data.name);
		}
	}
	return true;
}

//登録された全変数のUIを一括描画
void JsonSerializer::RenderGui()
{
	//登録変数の一括UI描画ループ
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];
		if (current_data.property_interface != nullptr)
		{
			current_data.property_interface->DrawImGui(current_data.name);
		}
	}
}