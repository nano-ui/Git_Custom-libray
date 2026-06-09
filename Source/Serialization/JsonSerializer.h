#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include "json.hpp"

//DirectXMathの型をJSONで自動変換するための定義
namespace nlohmann
{
	inline void to_json(json& json_data, const DirectX::XMFLOAT3& float3_data)
	{
		json_data = json{ {"x", float3_data.x}, {"y", float3_data.y}, {"z", float3_data.z} };
	}

	inline void fram_json(const json& json_data, DirectX::XMFLOAT3& float3_data)
	{
		json_data.at("x").get_to(float3_data.x);
		json_data.at("y").get_to(float3_data.y);
		json_data.at("z").get_to(float3_data.z);
	}
}

class IProperty
{
public:
	//デストラクタ
	virtual ~IProperty() = default;

	//値をJSONオブジェクトへ保存
	virtual void SeveTo(nlohmann::json& json_data, const std::string& property_name) = 0;
	
	//JSONオブジェクトから値を復元
	virtual void LoadFrom(const nlohmann::json& json_data, const std::string& property_name) = 0;
};

template <typename T>
class TypedProperty :public IProperty
{
public:
	//コンストラクタ
	TypedProperty(T* target_pointer)
	{
		data_pointer = target_pointer;
	}

	//デストラクタ
	~TypedProperty()override = default;

	//値をJSONオブジェクトへ保存
	void SeveTo(nlohmann::json& json_data, const std::string& property_name)override
	{
		json_data[property_name] = *data_pointer;
	}

	//JSONオブジェクトから値を復元
	void LoadFrom(const nlohmann::json& json_data, const std::string& property_name)override
	{
		if (json_data.contains(property_name))
		{
			*data_pointer = json_data[property_name].get<T>();
		}
	}


private:
	T* data_pointer;	//対象の変数
};

class JsonSerializer
{
public:
	//コンストラクタ
	JsonSerializer();

	//デストラクタ
	~JsonSerializer();

	//セーブ・ロードの自動処理対象にしたい変数名とポインタをシステムに登録
	template<typename T>
	void RegisterVariable(const std::string& property_name, T* target_variable)
	{
		PropertyData new_data;
		new_data.name = property_name;
		new_data.property_interface = std::make_unique<TypedProperty<T>>(target_variable);
		registered_properties.push_back(std::move(new_data));
	}

	//JSON形式でデータをファイルへ書き出す
	void SaveToFile(const std::string& file_path);

	//JSONファイルからデータを変数群へ読み込む
	void LoadFromFile(const std::string& file_path);

private:
	//個別のプロパティ情報をまとめる内部構造体
	struct PropertyData
	{
		std::string name;	//変数名
		std::unique_ptr<IProperty> property_interface;	//変数操作ポインタ
	};

private:
	std::vector<PropertyData> registered_properties;	//登録されたすべての変数データ
};

