#pragma once

#include <string>

class GameObject;
class ObjectEditor;
class StateMachineGraphEditor;
class ModelPreviewWindow;

class EditorMediator
{
public:
	//インスタンス取得
	static EditorMediator& Instance();

	//ステートマシンエディタのポインタを登録
	void RegisterStateMachineGraphEditor(StateMachineGraphEditor* editor);

	//モデルプレビューウィンドウのポインタを中継用に登録
	void RegisterModelPreviewWindow(ModelPreviewWindow* window);

	//オブジェクトエディタのポインタを仲介用に登録
	void RegisterObjectEditor(ObjectEditor* editor);

	//モデルファイルがドロップした際のイベント
	void OnModelDropped(const std::string& file_path);

	//中継通知イベント
	void OnObjectSelected(GameObject* object);

	//コンテンツブラウザ等でモデルファイルがダブルクリックされたときに呼び出し
	void OnModelDubleClied(const std::string& file_path);

	//再生速度を仲介
	void SetModelAnimationSpeed(float speed);

	//再生状態を仲介
	void SetModelAnimationPlaying(bool playing);

	//現在の再生時間を仲介
	void SetModelAnimationTime(float time);

	//モデルの現在の再生時間を取得
	float GetModelAnimationCurrentTime() const;

	//モデルのアニメーション総時間を取得
	float GetModelAnimationDuration() const;

	//モデルのファイルパスを取得
	std::string GetModelName()const;

	//現在再生されているアニメーション名を取得
	std::string GetModelAnimationName()const;

	//モデルのアニメーション再生を中継
	void PlayModelAnimation(const std::string& anim_name, bool is_loop);

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
	ModelPreviewWindow* model_preview_window = nullptr;				//モデルプレビュー
	ObjectEditor* object_editor = nullptr;							//オブジェクトエディタのポインタ
};

