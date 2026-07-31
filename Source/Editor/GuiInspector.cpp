#include "GuiInspector.h"
#include <Windows.h>

//コンストラクタ
GuiInspector::GuiInspector()
{
}

//デストラクタ
GuiInspector::~GuiInspector()
{
	registered_properties.clear();
}

//登録された全UIの一括描画処理
void GuiInspector::RenderGui()
{
	if (registered_properties.empty())return;
	std::vector<std::string> unique_categories;	//重複しないカテゴリリスト

	//重複を防ぎながらカテゴリ抽出
	for (size_t i = 0; i < registered_properties.size(); i++)
	{
		const PropertyData& current_data = registered_properties[i];
		bool is_duplicate = false;

		for (size_t j = 0; j < unique_categories.size(); j++)
		{
			if (unique_categories[j] == current_data.category)
			{
				is_duplicate = true;
				break;
			}
		}

		if (!is_duplicate)unique_categories.push_back(current_data.category);

		//カテゴリごとにグループ化して描画
		for (size_t cat_idx = 0; cat_idx < unique_categories.size(); cat_idx++)
		{
			const std::string& target_category = unique_categories[cat_idx];

			if (ImGui::CollapsingHeader(target_category.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				ImGui::PushID(target_category.c_str());

				for (size_t prop_idx = 0; prop_idx < registered_properties.size(); prop_idx++)
				{
					const PropertyData& current_data = registered_properties[prop_idx];

					if (current_data.category == target_category)
					{
						if (current_data.prorerty_interface)
						{
							ImGui::PushID(static_cast<int>(prop_idx));
							current_data.prorerty_interface->DrawImGui(current_data.name);
							ImGui::PopID();
						}
						else OutputDebugStringA("[GuiInspector エラー] property_interface が nullptr です。\n");
					}
				}
			}
			ImGui::PopID();
			ImGui::Unindent();
		}
	}
}