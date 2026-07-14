#pragma once

class ObjectEditor;

class EditorMenuBar
{
public:
	//コンストラクタ
	EditorMenuBar();

	//デストラクタ
	~EditorMenuBar();

	//初期化処理
	void Initialize();

	//Gui描画
	void RenderGui(ObjectEditor* object_editor);
};

