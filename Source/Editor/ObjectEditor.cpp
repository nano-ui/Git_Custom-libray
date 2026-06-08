#include "ObjectEditor.h"
#include "../GameObjects/ObjectFactory.h"
#include "../GameObjects/GameObject.h"

#include <imgui.h>

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
}

//描画
void ObjectEditor::RenderUi()
{
	//画面を左右に分割するメインウインドウ
	ImGui::Begin("Object Editor");
	ImGui::BeginChild("LeftPane", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
	DrawLeftPane();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
	DrawRightPane();
	ImGui::EndChild();

	ImGui::End();
}

//オブジェクト生成UI描画
void ObjectEditor::DrawLeftPane()
{
	//クラス名リストの取得とリストボックスの描画
	ImGui::Text("Registered Classes");
	ImGui::Separator();

	std::vector<std::string> class_names = ObjectFactory::GetClassNames();

	if (!class_names.empty())
	{
		if (ImGui::ListBoxHeader("##ClassList", ImVec2(-1, ImGui::GetContentRegionAvail().y - 40)))
		{
			for (int i = 0; i < static_cast<int>(class_names.size()); ++i) 
			{
				const bool is_selected = (selected_class_index == i);

				if (ImGui::Selectable(class_names[i].c_str(), is_selected))
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
		const std::string& target_class_name = class_names[selected_class_index];
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
			current_selected_object->RenderGui();
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Separator();

			//オブジェクトの削除ボタン
			if (ImGui::Button("Destroy Object", ImVec2(-1, 0)))
			{
				current_selected_object->Destory();
				current_selected_object = nullptr;
			}
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
