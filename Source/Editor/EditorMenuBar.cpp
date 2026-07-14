#include "EditorMenuBar.h"
#include "ObjectEditor.h"

#include <imgui.h>
#include <windows.h>

//コンストラクタ
EditorMenuBar::EditorMenuBar()
{
}

//デストラクタ
EditorMenuBar::~EditorMenuBar()
{
}

//初期化処理
void EditorMenuBar::Initialize()
{
}

//メニューバーのGUI描画処理
void EditorMenuBar::RenderGui(ObjectEditor* object_editor)
{
	//最上部のメインメニューバーの描画が開始できたか判定
	if (ImGui::BeginMainMenuBar())
	{
		//「ファイル」メニューの展開が選択されたか判定
		if (ImGui::BeginMenu(u8"ファイル"))
		{
			//「シーン保存」が選択されたか判定
			if (ImGui::MenuItem(u8"シーン保存"))
			{
				//オブジェクトエディタのポインタが有効か判定
				if (object_editor)
				{
					object_editor->SaveSceneWithDialog();
				}
				else
				{
					OutputDebugStringA("Error: object_editor is null in EditorMenuBar::RenderGui when saving\n");
				}
			}

			//「シーン読み込み」が選択されたか判定
			if (ImGui::MenuItem(u8"シーン読み込み"))
			{
				//オブジェクトエディタのポインタが有効か判定
				if (object_editor)
				{
					object_editor->LoadSceneWithDialog();
				}
				else
				{
					OutputDebugStringA("Error: object_editor is null in EditorMenuBar::RenderGui when loading\n");
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}