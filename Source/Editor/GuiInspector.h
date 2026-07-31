#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <type_traits>
#include <imgui.h>

//UI描画用インターフェース
class IGuiProperty
{
public:
	virtual ~IGuiProperty() = default;

	//ImGui描画
	virtual void DrawImGui(const std::string& prorerty_name) = 0;
};

//編集用プロパティテンプレート
template<typename T>
class TypedGuiProperty :public IGuiProperty
{
public:
	//コンストラクタ
	TypedGuiProperty(T* target_pointer) { data_pointer = target_pointer; }

	//デストラクタ
	~TypedGuiProperty()override = default;

	//ImGui描画
	void DrawImGui(const std::string& property_name)override
	{
		if (!data_pointer)return;

		//型に応じた関数をコンパイル時に結合
		if constexpr (std::is_same_v<T, int>)
		{
			constexpr float drag_speed = 1.0f;
			ImGui::DragInt(property_name.c_str(), data_pointer, drag_speed);
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
	T* data_pointer = nullptr;	//描画対象データポインタ
};

//読み取り用プロパティテンプレート
template<typename T>
class TextGuiProperty :public IGuiProperty
{
public:
	//コンストラクタ
	TextGuiProperty(const T* target_pointer) { data_pointer = target_pointer; }

	//デストラクタ
	~TextGuiProperty()override = default;

	//ImGui描画処理
	void DrawImGui(const std::string& property_name)override
	{
		if (!data_pointer)return;

		//テキスト出力
		if constexpr (std::is_same_v<T, int>)					ImGui::Text("%s: %d", property_name.c_str(), *data_pointer);
		else if constexpr (std::is_same_v<T, float>)			ImGui::Text("%s: %.3f", property_name.c_str(), *data_pointer);
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT3>)ImGui::Text("%s: (%.2f, %.2f, %.2f)", property_name.c_str(), data_pointer->x, data_pointer->y, data_pointer->z);
		else if constexpr (std::is_same_v<T, bool>)				ImGui::Text("%s: %s", property_name.c_str(), *data_pointer ? "true" : "false");
		else if constexpr (std::is_same_v<T, std::string>)		ImGui::Text("%s: %s", property_name.c_str(), data_pointer->c_str());
		else													ImGui::Text("%s: [Text Display Unsupported]", property_name.c_str());
	}

private:
	const T* data_pointer = nullptr;	//参照用ポインタ
};

class GuiInspector
{
public:
	//コンストラクタ
	GuiInspector();

	//デストラクタ
	~GuiInspector();

	//編集用変数を登録
	template <typename T>
	void RegisterVariable(const std::string& property_name, T* target_variable, const std::string& category_name = u8"デフォルト")
	{
		PropertyData new_data;
		new_data.name = property_name;
		new_data.category = category_name;
		new_data.prorerty_interface = std::make_unique<TypedGuiProperty<T>>(target_variable);
		registered_properties.push_back(std::move(new_data));
	}

	//表示用変数を登録
	template <typename T>
	void RegisterText(const std::string& property_name, T* target_variable, const std::string& category_name = u8"デフォルト")
	{
		PropertyData new_data;
		new_data.name = property_name;
		new_data.category = category_name;
		new_data.prorerty_interface = std::make_unique<TextGuiProperty<T>>(target_variable);
		registered_properties.push_back(std::move(new_data));
	}

	//登録された全UI要素の一括描画処理
	void RenderGui();

private:
	//個別のプロパティ情報
	struct PropertyData
	{
		std::string name;		//名
		std::string category;	//所属するグループ
		std::unique_ptr<IGuiProperty> prorerty_interface;	//操作ポインタ
	};

private:
	std::vector<PropertyData> registered_properties;	//登録UIコンテナ
};

