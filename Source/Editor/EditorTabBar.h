#pragma once

#include <string>

enum class EditorSceneType
{
	LevelEditor,		//レベル
	StateMachineEditor,	//ステートマシンエディタ
	AnimationSequencer,	//アニメーションシーケンサエディタ
};

class EditorTabBar
{
public:
	//コンストラクタ
	EditorTabBar();

	//デストラクタ
	~EditorTabBar();

	//初期化処理
	void Initalize();

	//遷移タブバー描画
	EditorSceneType Draw();

private:
	//各タブアイテムの描画と選択状態の更新
	void DrawTabItem(const char* label, EditorSceneType type, float width);

private:
	EditorSceneType current_scene_type;	//現在のエディタシーン
};

