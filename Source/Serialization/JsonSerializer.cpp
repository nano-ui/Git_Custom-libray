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

//JSONオブジェクトへ登録データを直接書き出す
void JsonSerializer::SaveToObject(nlohmann::json& root_json)
{
	//登録されているすべてのプロパティをループ処理してJSONノードへ変換
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];	//要素の参照

		if (current_data.property_interface != nullptr)
		{
			current_data.property_interface->SaveTo(root_json, current_data.name);
		}
	}
}

//JSONオブジェクトからデータを直接読み込む
void JsonSerializer::LoadFromObject(const nlohmann::json& root_json)
{
	//登録されているすべてのプロパティをループ処理して値を復元
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];	//要素の参照

		if (current_data.property_interface != nullptr)
		{
			current_data.property_interface->LoadFrom(root_json, current_data.name);
		}
	}
}

//登録された全変数のUIを一括描画
void JsonSerializer::RenderGui()
{
	std::vector<std::string> unique_categories;	//カテゴリ名コンテナ

	//登録変数の一括UI描画ループ
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];
		bool is_duplicate = false;

		//既に抽出済みのカテゴリか判定
		for (size_t j = 0; j < unique_categories.size(); j++)
		{
			if (unique_categories[j] == current_data.category)
			{
				is_duplicate = true;
				break;
			}
		}

		if (!is_duplicate)
		{
			unique_categories.push_back(current_data.category);
		}
	}

	//カテゴリごとに折り畳みヘッダーを生成して描画
	for (size_t cat_idx = 0; cat_idx < unique_categories.size(); cat_idx++)
	{
		const std::string& target_category = unique_categories[cat_idx];

		//カテゴリごとの折り畳みヘッダーが展開されるか判定
		if (ImGui::CollapsingHeader(target_category.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent();

			//現在のカテゴリに所属するプロパティだけImGuiに描画
			for (size_t prop_idx = 0; prop_idx < registered_properties.size(); prop_idx++)
			{
				const PropertyData& current_data = registered_properties[prop_idx];

				//プロパティのカテゴリーが一致しているか判定
				if (current_data.category == target_category)
				{
					if (current_data.property_interface != nullptr)
					{
						current_data.property_interface->DrawImGui(current_data.name);
					}
				}
			}
			ImGui::Unindent();
		}
	}

}