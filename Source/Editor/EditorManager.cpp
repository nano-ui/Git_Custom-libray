#include "EditorManager.h"
#include "ObjectEditor.h"
#include "StateMachineEditor\StateMachineGraphEditor.h"
#include "Sequence\AnimationSequencerEditor.h"
#include "EditorMenuBar.h"
#include "ConstentBrowser\ContentBrowserEditor.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Collision\CollisionManager.h"
#include "Gameplay\GameObjects\Character\Character.h"

#include <windows.h>

//コンストラクタ
EditorManager::EditorManager()
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

	object_editor->Initialize();
	animation_sequencer_editor->Initialize();
	content_browser_editor->Initialize();
}

//更新処理
void EditorManager::Update(float elapsed_time)
{

}

//Gui描画、レイアウト構築
void EditorManager::RenderGui(Camera* camera, CollisionManager* collision_manager)
{
	menu_bar->RenderGui(object_editor.get());

	ImGui::Begin(u8"エディタマネージャーウィンドウ");
	ImGui::Text(u8"エディタマネージャー有効");
	ImGui::End();

	//画面全体のドッキング空間用の設定フラグ構築
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	//ドッキングの背景となるウィンドウのスタイルを設定
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground; //ゲーム画面の上に重ねるため背景を透明化

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

	//各種エディタ呼び出し
	object_editor->RenderUi(camera, collision_manager);

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

	content_browser_editor->RenderGui();

	animation_sequencer_editor->RenderGui();
}
