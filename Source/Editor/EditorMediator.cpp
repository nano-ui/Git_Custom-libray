#include "EditorMediator.h"
#include "Gameplay/GameObjects/GameObject.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Gameplay/GameObjects/Character/Character.h"
#include "StateMachineEditor\StateMachineGraphEditor.h"
#include "Preview\ModelPreviewWindow.h"

#include <iostream>

//シングルトンインスタンスの取得
EditorMediator& EditorMediator::Instance()
{
	static EditorMediator instance;
	return instance;
}

//ステートマシンエディタのポインタを登録
void EditorMediator::RegisterStateMachineGraphEditor(StateMachineGraphEditor* editor)
{
	if (editor)
	{
		state_machine_graph_editor = editor;
	}
}

//モデルプレビューウィンドウのポインタを中継用に登録
void EditorMediator::RegisterModelPreviewWindow(ModelPreviewWindow* window)
{
	if (window)
	{
		model_preview_window = window;
	}
	else
	{
		OutputDebugStringA("[Warning] EditorMediator::RegisterModelPreviewWindow: Passed window is null!\n");
	}
}

//オブジェクトエディタのポインタを仲介用に登録
void EditorMediator::RegisterObjectEditor(ObjectEditor* editor)
{
	if (editor)object_editor = editor;
	else OutputDebugStringA("[Warning] EditorMediator::RegisterObjectEditor: Passed editor is null!\n");
}

//モデルファイルがドロップした際のイベント
void EditorMediator::OnModelDropped(const std::string& file_path)
{
	if (object_editor)return;
	else OutputDebugStringA("[Error] EditorMediator: ObjectEditor is not registered!\n");
}

//中継通知イベント
void EditorMediator::OnObjectSelected(GameObject* object)
{
	if (!object || object == last_selected_object)
	{
		return;
	}

	last_selected_object = object;

	if (!state_machine_graph_editor)
	{
		std::cerr << "[Warning] EditorMediator::OnObjectSelected - StateMachineGraphEditor が登録されていません。\n";
		return;
	}

	Character* character_ptr = dynamic_cast<Character*>(object);
	if (character_ptr)
	{
		if (character_ptr->GetStateMachineComponent())
		{
			std::string target_path = character_ptr->GetStateMachineComponent()->GetStateMachinePath();

			if (!target_path.empty())
			{
				state_machine_graph_editor->LoadGraphFromFile(target_path);
				printf("EditorMediator: オブジェクト「%s」の選択変更を検知し、エディタを「%s」へ自動同期しました。\n",
					object->GetClassName().c_str(), target_path.c_str());
			}
		}
	}
}

//コンテンツブラウザ等でモデルファイルがダブルクリックされたときに呼び出し
void EditorMediator::OnModelDubleClied(const std::string& file_path)
{
	if (model_preview_window)
	{
		model_preview_window->LoadModel(file_path);
	}
	else
	{
		OutputDebugStringA("[Error] EditorMediator: ModelPreviewWindow is not registered! Cannot load model.\n");
	}
}

//再生速度を仲介
void EditorMediator::SetModelAnimationSpeed(float speed)
{
	model_preview_window->SetAnimationSpeed(speed);
}

//再生状態を仲介
void EditorMediator::SetModelAnimationPlaying(bool playing)
{
	model_preview_window->SetPlaying(playing);
}

//現在の再生時間を仲介
void EditorMediator::SetModelAnimationTime(float time)
{
	model_preview_window->SetAnimationTime(time);
}

//モデルの現在の再生時間を取得
float EditorMediator::GetModelAnimationCurrentTime() const
{
	if (model_preview_window)return model_preview_window->GetAnimationCurrentTime();
}

//モデルのアニメーション総時間を取得
float EditorMediator::GetModelAnimationDuration() const
{
	if (model_preview_window)return model_preview_window->GetAnimationDuration();
}

//モデルのファイルパスを取得
std::string EditorMediator::GetModelName() const
{
	if (model_preview_window) return model_preview_window->GetModelName();
}

//現在再生されているアニメーション名を取得
std::string EditorMediator::GetModelAnimationName() const
{
	if (model_preview_window)return model_preview_window->GetAnimationName();
}

//モデルのアニメーション再生を中継
void EditorMediator::PlayModelAnimation(const std::string& anim_name, bool is_loop)
{
	if (model_preview_window)model_preview_window->PlayPreviewAnimation(anim_name, is_loop);
	else
	{
		printf("Error: EditorMediator::PlayModelAnimation - ModelPreviewWindow が登録されていません。\n");
	}
}

//アクティブノードIDをエディタに同期
void EditorMediator::UpdateViewerSynchronization(GameObject* object)
{
	if (!object || !state_machine_graph_editor)
	{
		return;
	}

	Character* character_ptr = dynamic_cast<Character*>(object);

	if (character_ptr)
	{
		StateMachineComponent* sm_comp = character_ptr->GetStateMachineComponent();
		if (sm_comp)
		{
			uint32_t active_node_id = sm_comp->GetCurrentNodeId();
			state_machine_graph_editor->SetRuntimeActiveNodeId(active_node_id);
		}
	}

}

//対象コンポーネントへリロード命令を仲介
void EditorMediator::NotifyGraphChanged(const std::string& file_path)
{
	if (file_path.empty() || !last_selected_object)
	{
		return;
	}

	Character* character_ptr = dynamic_cast<Character*>(last_selected_object);
	if (character_ptr)
	{
		StateMachineComponent* sm_comp = character_ptr->GetStateMachineComponent();
		if (sm_comp)
		{
			sm_comp->RequestReload();
		}
	}
}

//コンストラクタ
EditorMediator::EditorMediator()
{
	state_machine_graph_editor = nullptr;
	last_selected_object = nullptr;
}

//デストラクタ
EditorMediator::~EditorMediator()
{

}

