#include "EditorManager.h"
#include "ObjectEditor.h"
#include "StateMachineEditor\StateMachineGraphEditor.h"
#include "Sequence\AnimationSequencerEditor.h"
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
}

//更新処理
void EditorManager::Update(float elapsed_time)
{

}

//Gui描画、レイアウト構築
void EditorManager::RenderGui(Camera* camera, CollisionManager* collision_manager)
{
	ImGui::Begin(u8"エディタマネージャーウィンドウ");
	ImGui::Text(u8"エディタマネージャー有効");
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

	animation_sequencer_editor->RenderGui();
}
