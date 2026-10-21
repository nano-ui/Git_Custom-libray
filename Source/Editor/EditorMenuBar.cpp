#include "EditorMenuBar.h"
#include "ObjectEditor.h"
#include "Log\LogEditor.h"

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
void EditorMenuBar::RenderGui(ObjectEditor* object_editor, LogEditor* log_editor)
{
	if (!object_editor || !log_editor)
	{
		OutputDebugStringA("[EditorMenuBar::RenderGui] 引数がnullです。\n");
		return;
	}

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
				object_editor->SaveSceneWithDialog();
			}

			//「シーン読み込み」が選択されたか判定
			if (ImGui::MenuItem(u8"シーン読み込み"))
			{
				//オブジェクトエディタのポインタが有効か判定
				object_editor->LoadSceneWithDialog();
				OutputDebugStringA("Error: object_editor is null in EditorMenuBar::RenderGui when loading\n");
			}

			ImGui::EndMenu();
		}

		ImGui::SameLine();

		if (ImGui::BeginMenu(u8"表示"))
		{
			ImGui::MenuItem(u8"ログ", nullptr, log_editor->GetOpenFlagAddress());
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}