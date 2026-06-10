#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <type_traits>
#include <imgui.h>
#include "json.hpp"

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

	//ImGui描画
	virtual void DrawImGui(const std::string& property_name) = 0;
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

	//ImGui描画
	void DrawImGui(const std::string& property_name)override
	{
		//型に応じた関数をコンパイル時に結合
		if constexpr(std::is_same_v<T, int>)
		{
			constexpr float drag_speed = 1.0f;
			ImGui::DragInt(property_name.c_str(),data_pointer,drag_speed);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			constexpr float drag_speed = 0.1f;
			ImGui::DragFloat(property_name.c_str(), data_pointer, drag_speed);
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT3>)
		{
			constexpr float drag_speed = 0.1f;
			ImGui::DragFloat3(property_name.c_str(), &data_pointer->x, drag_speed);
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4>)
		{
			constexpr float drag_speed = 0.1f;
			ImGui::DragFloat4(property_name.c_str(), &data_pointer->x, drag_speed);
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			ImGui::Checkbox(property_name.c_str(), data_pointer);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			constexpr size_t string_buffer_size = 256;
			char edit_buffer[string_buffer_size];

			strncpy_s(edit_buffer, string_buffer_size, data_pointer->c_str(), _TRUNCATE);

			if (ImGui::InputText(property_name.c_str(), edit_buffer, string_buffer_size))
			{
				*data_pointer = edit_buffer;
			}
		}
		else
		{
			ImGui::Text("Unsupported Type: %s", property_name.c_str());
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
	bool LoadFromFile(const std::string& file_path);

	//登録された全変数のUIを一括描画
	void RenderGui();

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