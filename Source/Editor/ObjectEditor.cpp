#include "ObjectEditor.h"
#include "../GameObjects/ObjectFactory.h"
#include "../GameObjects/GameObject.h"
#include "../GameObjects/ObjectManager.h"
#include "../Collision/CollisionManager.h"

#include <imgui.h>
#include <string>
#include <unordered_map>

constexpr float dummy_height_value = 10.0f;
constexpr float class_list_height_ratio = 0.3f;
constexpr float active_list_height_offset = 60.0f;
constexpr size_t label_buffer_size = 128;
constexpr float panel_width_ratio = 0.2f;

//コンストラクタ
ObjectEditor::ObjectEditor()
{
	selected_class_index = 0;
	current_selected_object = nullptr;
}

//デストラクタ
ObjectEditor::~ObjectEditor()
{

}

//初期化
void ObjectEditor::Initialize()
{
	selected_class_index = 0;
	current_selected_object = nullptr;
	cached_class_names = ObjectFactory::GetClassNames();
}

//描画
void ObjectEditor::RenderUi()
{
	//画面解像度に基づいた初期配置位置の計算
	ImGuiIO io = ImGui::GetIO();
	const float screen_width = io.DisplaySize.x;
	const float screen_height = io.DisplaySize.y;
	const float panel_width = screen_width * panel_width_ratio;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(panel_width, screen_height), ImGuiCond_FirstUseEver);

	//画面を左右に分割するメインウインドウ
	ImGui::Begin("Hierarchy");
	DrawLeftPane();
	ImGui::End();

	const float right_window_pos_x = screen_width - panel_width;
	ImGui::SetNextWindowPos(ImVec2(right_window_pos_x, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(panel_width, screen_height), ImGuiCond_FirstUseEver);

	ImGui::Begin("Inspector");
	DrawRightPane();
	ImGui::End();
}

//オブジェクト生成UI描画
void ObjectEditor::DrawLeftPane()
{
	//クラス名リストの取得とリストボックスの描画
	ImGui::Text("Registered Classes");
	ImGui::Separator();

	if (!cached_class_names.empty())
	{
		if (ImGui::ListBoxHeader("##ClassList", ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.3f)))
		{
			for (int i = 0; i < static_cast<int>(cached_class_names.size()); ++i)
			{
				const bool is_selected = (selected_class_index == i);

				if (ImGui::Selectable(cached_class_names[i].c_str(), is_selected))
				{
					selected_class_index = i;
				}

				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::ListBoxFooter();
		}
	}

	//生成ボタン処理
	if (ImGui::Button("Create and Register", ImVec2(-1, 0))) 
	{
		const std::string& target_class_name = cached_class_names[selected_class_index];
		GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
		if (new_object)
		{
			new_object->Initialize();
			current_selected_object = new_object;
		}
	}
	else
	{
		ImGui::Text("No class registered in ObjectFactory");
	}
	ImGui::Dummy(ImVec2(0.0f, dummy_height_value));

	//現在生成されているオブジェクトの一覧リスト描画
	ImGui::Text("Active Object List");
	ImGui::Separator();
	const auto& active_objects = ObjectManager::Instance().GetGameObjects();
	frame_class_counters.clear();

	if (ImGui::ListBoxHeader("##ActiveObjects", ImVec2(-1, ImGui::GetContentRegionAvail().y - active_list_height_offset))) 
	{
		for (size_t i = 0; i < active_objects.size(); ++i)
		{
			if (active_objects[i]->IsActive())
			{
				GameObject* obj_ptr = active_objects[i].get();
				std::string current_class_name = obj_ptr->GetClassName();
				int current_number = frame_class_counters[current_class_name];
				frame_class_counters[current_class_name]++;
				ImGui::PushID(reinterpret_cast<const void*>(obj_ptr));

				char label_buffer[label_buffer_size];
				snprintf(label_buffer, sizeof(label_buffer), "%s %d", current_class_name.c_str(), current_number);
				const bool is_current = (current_selected_object == obj_ptr);
				if (ImGui::Selectable(label_buffer, is_current))
				{
					current_selected_object = obj_ptr;
				}

				if (is_current) 
				{
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}
		}
		ImGui::ListBoxFooter();
	}
}

//オブジェクトパラメータ編集UI描画
void ObjectEditor::DrawRightPane()
{
	//選択中のオブジェクト情報の表示
	ImGui::Text("Object Properties");
	ImGui::Separator();

	if (current_selected_object != nullptr)
	{
		if (current_selected_object->IsActive())
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Separator();

			//オブジェクトの削除ボタン
			if (ImGui::Button("Destroy Object", ImVec2(-1, 0)))
			{
				current_selected_object->Destory();
				current_selected_object = nullptr;
				return;
			}
			ImGui::Dummy(ImVec2(0.0f, dummy_height_value));
			ImGui::Separator();
			current_selected_object->RenderGui();
		}
		else
		{
			ImGui::Text("The selected object has been destroyed.");
			current_selected_object = nullptr;
		}
	}
	else
	{
		ImGui::Text("Please select or create an object from the left pane.");
	}
}
