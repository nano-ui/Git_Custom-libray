#pragma once

#include <vector>
#include <string>

class GameObject;

class ObjectEditor
{
public:
	//コンストラクタ
	ObjectEditor();

	//デストラクタ
	~ObjectEditor();

	//初期化
	void Initialize();

	//描画
	void RenderUi();

private:
	//オブジェクト生成UI描画
	void DrawLeftPane();

	//オブジェクトパラメータ編集UI描画
	void DrawRightPane();

private:
	int selected_class_index;				//選択されているクラスのインデックス
	GameObject* current_selected_object;	//選択されているゲームオブジェクト
};

