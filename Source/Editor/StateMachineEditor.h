#pragma once

#include <string>
#include <vector>
#include <memory>

#include "StateNodeData.h"

class JsonSerializer;

class StateMachineEditor
{
public:
	//コンストラクタ
	StateMachineEditor();

	//デストラクタ
	~StateMachineEditor();

	//初期化
	void Initialize();

	//描画
	void RenderGui();

	//モデルのアニメーション名リストを設定
	void SetAvailableAnimations(const std::vector<std::string>& anim_list) { available_animations = anim_list; }

private:
	//メニューバー
	void DrawMenuBar();

	//登録ステートの一覧・追加・削除
	void DrawStateListPane();

	//選択中のステート・遷移条件のプロパティ編集
	void DrawInspectorPane();

	//JSONに書き出す
	void SaveEditorData(const std::string& file_path);

	//JSONファイルから編集データを読み込む
	void LoadEditorData(const std::string& file_path);

private:
	std::vector<StateNodeData> editor_states;			//エディタ上で現在編集している全ステートのデータ配列
	int selected_state_index;							//現在選択しているステートの添え字番号
	std::string initial_state_name;						//ゲーム開始時に最初に有効にする初期ステート名
	std::unique_ptr<JsonSerializer> json_serializer;	//変数の自動シリアライズ/ImGui描画を任せる
	std::vector<std::string> available_animations;		//エディタ内で選択肢として使用するアニメーション名の一覧配列
};

