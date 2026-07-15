#pragma once

#include"EditorTabBar.h"

#include <memory>

class ObjectEditor;
class ContentBrowserEditor;
class StateMachineGraphEditor;
class AnimationSequencerEditor;
class EditorMenuBar; 
class Camera;
class CollisionManager;

class EditorManager
{
public:
	//コンストラクタ
	EditorManager();

	//デストラクタ
	~EditorManager();

	//各種エディタの初期化
	void Initialize();

	//更新処理
	void Update(float elapsed_time);

	//Gui描画、レイアウト構築
	void RenderGui(Camera* camera, CollisionManager* collision_manager);

	//現在ゲーム画面がアクティブかどうかを返す
	bool IsGameViewportActive() const { return active_scene_type == EditorSceneType::LevelEditor; }

private:
	std::unique_ptr<ObjectEditor> object_editor;							//オブジェクトエディタ
	std::unique_ptr<StateMachineGraphEditor> state_graph_editor;			//ステートマシンエディタ
	std::unique_ptr<AnimationSequencerEditor> animation_sequencer_editor;	//アニメーションシーケンサエディタ
	std::unique_ptr<EditorMenuBar> menu_bar;								//メニューバーエディタ
	std::unique_ptr<ContentBrowserEditor> content_browser_editor;			//コンテンツブラウザエディタ
	std::unique_ptr<EditorTabBar> tab_bar;									//アセットタブバー
	EditorSceneType active_scene_type;										//現在アクティブなエディタ画面の種類
};

