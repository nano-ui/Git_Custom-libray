#include "EditorManager.h"
#include "ObjectEditor.h"
#include "StateMachineEditor\StateMachineGraphEditor.h"
#include "Sequence\AnimationSequencerEditor.h"
#include "EditorMenuBar.h"
#include "ConstentBrowser\ContentBrowserEditor.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Collision\CollisionManager.h"
#include "Preview\ModelPreviewWindow.h"
#include "EditorMediator.h"
#include "Gameplay\GameObjects\Character\Character.h"

#include <windows.h>

//コンストラクタ
EditorManager::EditorManager()
	:active_scene_type(EditorSceneType::LevelEditor)
{
}

//デストラクタ
EditorManager::~EditorManager()
{
}

//各種エディタの初期化
void EditorManager::Initialize()
{
	//各エディタインスタンスの生成
	object_editor = std::make_unique<ObjectEditor>();
	state_graph_editor = std::make_unique<StateMachineGraphEditor>();
	animation_sequencer_editor = std::make_unique<AnimationSequencerEditor>();
	menu_bar = std::make_unique<EditorMenuBar>();
	content_browser_editor = std::make_unique<ContentBrowserEditor>();
	tab_bar = std::make_unique<EditorTabBar>();
	model_preview_window = std::make_unique<ModelPreviewWindow>();

	object_editor->Initialize();
	animation_sequencer_editor->Initialize();
	content_browser_editor->Initialize();
	tab_bar->Initialize();
	model_preview_window->Initialize();
	EditorMediator::Instance().RegisterModelPreviewWindow(model_preview_window.get());
}

//更新処理
void EditorManager::Update(float elapsed_time)
{
	model_preview_window->Update(elapsed_time);
	animation_sequencer_editor->Update(elapsed_time);
}

//Gui描画、レイアウト構築
void EditorManager::RenderGui(Camera* camera, CollisionManager* collision_manager)
{
	menu_bar->RenderGui(object_editor.get());
	active_scene_type = tab_bar->Draw();

	//画面全体のドッキング空間用の設定フラグ構築
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	//タブバーの高さ分、DockSpaceの開始Y座標を下げる補正
	static constexpr float tab_bar_offset_y = 35.0f;
	ImVec2 dockspace_pos = viewport->WorkPos;
	dockspace_pos.y += tab_bar_offset_y;
	ImVec2 dockspace_size = viewport->WorkSize;
	dockspace_size.y -= tab_bar_offset_y;

	ImGui::SetNextWindowPos(dockspace_pos);
	ImGui::SetNextWindowSize(dockspace_size);
	ImGui::SetNextWindowViewport(viewport->ID);

	//ドッキングの背景となるウィンドウのスタイルを設定
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	if (active_scene_type == EditorSceneType::LevelEditor)
	{
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	//背景となる親ウィンドウの描画開始
	ImGui::Begin("EditorManagerDockSpaceWindow", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	//ドッキング空間の有効化
	ImGuiIO& io = ImGui::GetIO();

	//ドッキング機能がサポートされているか判定
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyEditorDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	}
	else
	{
		OutputDebugStringA("Warning: ImGuiConfigFlags_DockingEnable is not set.\n");
	}
	ImGui::End();


	switch (active_scene_type)
	{
	case EditorSceneType::LevelEditor:
	{
		object_editor->RenderUi(camera, collision_manager);
		content_browser_editor->RenderGui();
		break;
	}
	case EditorSceneType::StateMachineEditor:
	{
		StateBlackboard* target_blackboard = nullptr;
		GameObject* selected_obj = object_editor->GetCurrentSelectObject();
		if (selected_obj)
		{
			Character* selected_character = dynamic_cast<Character*>(selected_obj);
			if (selected_character)
			{
				target_blackboard = selected_character->GetBlackboard();
			}
		}
		state_graph_editor->DrawEditor(target_blackboard);
		model_preview_window->RenderGui();
		content_browser_editor->RenderGui();
		break;
	}
	case EditorSceneType::AnimationSequencer:
	{
		animation_sequencer_editor->RenderGui();
		model_preview_window->RenderGui();
		content_browser_editor->RenderGui();
		break;
	}
	default:
		OutputDebugStringA("[Error] EditorManager::RenderGui: Unknown active_scene_type detected!\n");
		break;
	}
}

//プレビュー用のフレームバッファに3Dモデルをレンダリング
void EditorManager::RenderPreviews(ID3D11DeviceContext* immediate_context)
{
	model_preview_window->Render(immediate_context);
}
