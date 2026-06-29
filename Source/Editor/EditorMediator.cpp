#include "EditorMediator.h"
#include "../Gameplay/GameObjects/GameObject.h"
#include "../Gameplay/Components/StateMachineComponent.h"
#include "../Gameplay/GameObjects/Character/Character.h"
#include "StateMachineGraphEditor.h"

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

