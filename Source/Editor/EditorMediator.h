#pragma once

#include <string>

class GameObject;
class StateMachineGraphEditor;

class EditorMediator
{
public:
	//インスタンス取得
	static EditorMediator& Instance();

	//ステートマシンエディタのポインタを登録
	void RegisterStateMachineGraphEditor(StateMachineGraphEditor* editor);

	//中継通知イベント
	void OnObjectSelected(GameObject* object);

	//アクティブノードIDをエディタに同期
	void UpdateViewerSynchronization(GameObject* object);

	//対象コンポーネントへリロード命令を仲介
	void NotifyGraphChanged(const std::string& file_path);

private:
	//外部での生成を禁止
	EditorMediator();

	//デストラクタ
	~EditorMediator();

private:
	StateMachineGraphEditor* state_machine_graph_editor = nullptr;	//対象エディタのポインタ
	GameObject* last_selected_object = nullptr;						//前回選択オブジェクト
};

