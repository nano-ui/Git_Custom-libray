#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <type_traits>
#include <imgui.h>
#include "ThiedParty\json.hpp"

//保存区分
enum class SerializeTarget : uint8_t 
{
	Base = 1 << 0,			//基底情報
	Detail = 1 << 1,		//固有情報
	All = Base | Detail		//共通情報
};

inline SerializeTarget operator&(SerializeTarget a, SerializeTarget b)
{
	return static_cast<SerializeTarget>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline SerializeTarget operator|(SerializeTarget a, SerializeTarget b)
{
	return static_cast<SerializeTarget>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

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

	//登録データのリセット処理
	void Clear() { registered_properties.clear(); }

	//セーブ・ロード対象の変数名とポインタを登録
	template<typename T>
	void RegisterVariable(const std::string& property_name, T* target_variable, SerializeTarget target = SerializeTarget::All)
	{
		PropertyData new_data;
		new_data.name = property_name;
		new_data.target = target;
		new_data.property_interface = std::make_unique<TypedProperty<T>>(target_variable);
		registered_properties.push_back(std::move(new_data));
	}

	//JSON形式でデータをファイルへ書き出す
	void SaveToFile(const std::string& file_path, SerializeTarget target = SerializeTarget::All);

	//JSONファイルからデータを群へ読み込む
	bool LoadFromFile(const std::string& file_path, SerializeTarget target = SerializeTarget::All);

	//JSONオブジェクトへ登録データを直接書き出す
	void SaveToObject(nlohmann::json& root_json, SerializeTarget target = SerializeTarget::All);

	//JSONオブジェクトからデータを直接読み込む
	void LoadFromObject(const nlohmann::json& root_json, SerializeTarget target = SerializeTarget::All);

private:
	//個別のプロパティ情報をまとめる内部構造体
	struct PropertyData
	{
		std::string name;		//名
		SerializeTarget target = SerializeTarget::All;	//保存タイプ
		std::unique_ptr<IProperty> property_interface;	//操作ポインタ
	};

private:
	std::vector<PropertyData> registered_properties;	//登録されたすべてのデータ
};