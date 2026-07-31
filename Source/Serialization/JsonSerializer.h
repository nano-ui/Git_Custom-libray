#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <type_traits>
#include <imgui.h>
#include "ThiedParty\json.hpp"

//DirectXMathの型をJSONで自動変換するための定義
namespace nlohmann
{
	inline void to_json(json& json_data, const DirectX::XMFLOAT3& float3_data)
	{
		json_data = json{ {"x", float3_data.x}, {"y", float3_data.y}, {"z", float3_data.z} };
	}

	inline void from_json(const json& json_data, DirectX::XMFLOAT3& float3_data)
	{
		json_data.at("x").get_to(float3_data.x);
		json_data.at("y").get_to(float3_data.y);
		json_data.at("z").get_to(float3_data.z);
	}

	inline void to_json(nlohmann::json& json_data, const DirectX::XMFLOAT4& float4_data)
	{
		json_data = nlohmann::json{ {"x", float4_data.x}, {"y", float4_data.y}, {"z", float4_data.z}, {"w", float4_data.w} };
	}

	inline void from_json(const nlohmann::json& json_data, DirectX::XMFLOAT4& float4_data)
	{
		json_data.at("x").get_to(float4_data.x);
		json_data.at("y").get_to(float4_data.y);
		json_data.at("z").get_to(float4_data.z);
		json_data.at("w").get_to(float4_data.w);
	}
}

class IProperty
{
public:
	//デストラクタ
	virtual ~IProperty() = default;

	//値をJSONオブジェクトへ保存
	virtual void SaveTo(nlohmann::json& json_data, const std::string& property_name) = 0;
	
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
	void SaveTo(nlohmann::json& json_data, const std::string& property_name)override
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
	T* data_pointer;	//対象のポインタ
};

class JsonSerializer
{
public:
	//コンストラクタ
	JsonSerializer();

	//デストラクタ
	~JsonSerializer();

	//JSON形式でデータをファイルへ書き出す
	void SaveToFile(const std::string& file_path);

	//JSONファイルからデータを群へ読み込む
	bool LoadFromFile(const std::string& file_path);

	//JSONオブジェクトへ登録データを直接書き出す
	void SaveToObject(nlohmann::json& root_json);

	//JSONオブジェクトからデータを直接読み込む
	void LoadFromObject(const nlohmann::json& root_json);

private:
	//個別のプロパティ情報をまとめる内部構造体
	struct PropertyData
	{
		std::string name;		//名
		std::unique_ptr<IProperty> property_interface;	//操作ポインタ
	};

private:
	std::vector<PropertyData> registered_properties;	//登録されたすべてのデータ
};