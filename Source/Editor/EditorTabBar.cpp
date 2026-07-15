#include "EditorTabBar.h"

#include <imgui.h>
#include <windows.h>

static constexpr float tab_button_width = 150.0f;
static constexpr float tab_bar_hieght = 35.0f;

//コンストラクタ
EditorTabBar::EditorTabBar()
	:current_scene_type(EditorSceneType::LevelEditor)
{
}

//デストラクタ
EditorTabBar::~EditorTabBar()
{
}

//初期化処理
void EditorTabBar::Initialize()
{
	current_scene_type = EditorSceneType::LevelEditor;
}

//遷移タブバー描画
EditorSceneType EditorTabBar::Draw()
{
	//メインメニューバーのすぐ下にタブバーを描画するためのウィンドウ設定
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, tab_bar_hieght));
	ImGui::SetNextWindowViewport(viewport->ID);

	//タイトルバーや背景を消し、平坦なバーとして描画
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar;

	if (ImGui::Begin("##EditorTabBarWindow", nullptr, window_flags)) 
	{
		if (ImGui::BeginMenuBar())
		{
			DrawTabItem(u8"レベル", EditorSceneType::LevelEditor, tab_button_width);
			DrawTabItem(u8"ステートマシン", EditorSceneType::StateMachineEditor, tab_button_width);
			DrawTabItem(u8"シーケンサ", EditorSceneType::AnimationSequencer, tab_button_width);
			ImGui::EndMenuBar();
		}
	}
	ImGui::End();

	return current_scene_type;
}

//各タブアイテムの描画と選択状態の更新
void EditorTabBar::DrawTabItem(const char* label, EditorSceneType type, float width)
{
	bool is_active = (current_scene_type == type);

	//アクティブなタブは少し明るい色にして視覚的に判別しやすく
	if (is_active)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
	}

	//タブ切り替えボタンの描画
	if (ImGui::Button(label, ImVec2(width, 0.0f)))
	{
		if (current_scene_type != type)
		{
			current_scene_type = type;
		}
	}
	ImGui::PopStyleColor();
}
