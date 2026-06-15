#pragma once

#include <string>
#include <vector>
#include <memory>
#include <windows.h>

#include "StateNodeData.h"

class JsonSerializer;
class Model;
class ObjectManager;

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
	void RenderGui(class ObjectManager* object_manager);

	//モデルのアニメーション名リストを設定
	void SetAvailableAnimations(const std::vector<std::string>& anim_list) { available_animations = anim_list; }

private:
	//メニューバー
	void DrawMenuBar(class ObjectManager* object_manager);

	//登録ステートの一覧・追加・削除
	void DrawStateListPane();

	//グラフ無限キャンパスの描画
	void DrawNodeGraphCanves();

	//すべてのノードの描画とドラッグ処理
	void DrawGraphNodes(ImDrawList* draw_list, ImVec2 canvas_pos);

	//ノード同士を繋ぐ矢印線の描画
	void DrawGraphTransitions(ImDrawList* draw_list, ImVec2 canvas_pos);

	//選択中のステート・遷移条件のプロパティ編集
	void DrawInspectorPane();

	//ファイルダイアログを開いてパスを取得
	std::string OpenFileDialog(bool is_save_mode, const char* filter);

	//JSONに書き出す
	void SaveEditorData(const std::string& file_path);

	//JSONファイルから編集データを読み込む
	void LoadEditorData(const std::string& file_path);

	//プレビューモデルの個別読込
	void LoadPreviewModel(const std::string& glb_path);

	//特定のクラスへ編集中のデータを同期
	void ApplyStateMachineToClass(class ObjectManager* object_manager, const std::string& target_class_name);

private:
	std::vector<StateNodeData> editor_states;			//エディタ上で現在編集している全ステートのデータ配列
	int selected_state_index;							//現在選択しているステートの添え字番号
	std::string initial_state_name;						//ゲーム開始時に最初に有効にする初期ステート名
	std::unique_ptr<JsonSerializer> json_serializer;	//変数の自動シリアライズ/ImGui描画を任せる
	std::vector<std::string> available_animations;		//エディタ内で選択肢として使用するアニメーション名の一覧配列
	std::string current_project_file_path;				//現在開いている、または保存対象のファイルパス
	std::unique_ptr<Model> preview_model;				//読み込んだモデル
	std::string loaded_model_path;						//読み込んだモデルのパス
	std::vector<std::string> scannable_class_names;		//シーン内から見つかった、適用可能なクラス名一覧
	int selected_target_class_index;					//適用先のクラス番号
	ImVec2 scrolling_offset = ImVec2(0.0f, 0.0f);		//キャンバスのスクロール位置（マウス中クリックドラッグで画面移動）
	float zoom_factor = 1.0f;							//キャンバスの拡大縮小倍率
	int link_source_node_index = -1;					//新しい矢印線を引っ張る際の「開始ノード」の番号
	int selected_transition_src_index = -1;				//現在選択されている矢印の出発元ノード番号
	int selected_transition_idx = -1;					//現在選択されている矢印の配列内インデックス
};