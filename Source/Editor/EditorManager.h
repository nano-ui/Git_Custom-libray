#pragma once

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

private:
	std::unique_ptr<ObjectEditor> object_editor;							//オブジェクトエディタ
	std::unique_ptr<StateMachineGraphEditor> state_graph_editor;			//ステートマシンエディタ
	std::unique_ptr<AnimationSequencerEditor> animation_sequencer_editor;	//アニメーションシーケンサエディタ
	std::unique_ptr<EditorMenuBar> menu_bar;								//メニューバーエディタ
	std::unique_ptr<ContentBrowserEditor> content_browser_editor;			//コンテンツブラウザエディタ
};

