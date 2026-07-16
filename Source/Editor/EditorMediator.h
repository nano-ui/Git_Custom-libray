#pragma once

#include <string>

class GameObject;
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
};

