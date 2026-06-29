#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <imgui.h>
#include <unordered_map>

class StateMachine;
class StateBlackboard;
class StateGraphDataManager;
class StateGraphPaletteWindow;
class StateGraphPropertyWindow;
class StateGraphSimulator;
class StateGraphConfigManager;
class AssetLoader;

struct GraphData;
struct GraphLink;

namespace ax { namespace NodeEditor { struct EditorContext; } }

class StateMachineGraphEditor
{
public:
	//コンストラクタ
	StateMachineGraphEditor();

	//デストラクタ
	~StateMachineGraphEditor();

	//エディタ描画
	void DrawEditor(StateBlackboard* blackboard);

	//ファイルパスのグラフ情報をリロード
	bool LoadGraphFromFile(const std::string& file_path);

	//アクティブノードID設定
	void SetRuntimeActiveNodeId(uint32_t node_id) { runtime_active_node_id = node_id; }

private:

	//サブグラフへの階層移動を検知・処理
	void CheckNavigateToSubGraph(GraphData* current_graph);

	//階層ナビゲーションを描画
	bool DrawHeaderNavigation();

	//ノードの削除
	void DeleteNode(GraphData* current_graph);

	//接続線の削除
	void DeleteLink(GraphData* current_graph);

	//接続線の作成を検知してデータに追加
	void CreateNewLink(GraphData* current_graph);

	//遷移条件を構築
	void OnLinkCreated(GraphData* current_graph, const GraphLink& new_link);

	//最後に使用したファイルパスを設定ファイルへ保存
	void SaveEditorCondig();

	//設定ファイルから最後に使用したファイルパスを読み込む
	void LoadEditorCondig();

	//アニメーションマップを構築して送信
	void TriggerHotReload();

private:
	//カスタムデリータ
	struct EditorContexDeleter
	{
		void operator()(ax::NodeEditor::EditorContext* context)const noexcept;
	};

private:
	std::unique_ptr<StateGraphDataManager> data_manager;				//データを専門的に扱うマネージャー
	std::unique_ptr<StateGraphPaletteWindow> palette_window;			//左ペイン：パレット描画クラス
	std::unique_ptr<StateGraphPropertyWindow> property_window;			//右ペイン：プロパティ描画クラス
	std::unique_ptr<ax::NodeEditor::EditorContext, EditorContexDeleter> editor_context;	//エディタのライフサイクルを管理
	std::unique_ptr<StateGraphConfigManager> config_manager;
	std::unique_ptr<AssetLoader> asset_loader;					//モデル読み込みクラス

	uint32_t current_graph_id;									//現在の階層のグラフID
	std::unordered_map<uint32_t, uint32_t> graph_active_nodes;	//各階層ごとのアクティブノードIDを個別に保持
	uint32_t previous_active_node_id = 0;						//前フレームのアクティブノードID
	uint32_t current_active_node_id = 0;						//現在のアクティブノードID
	std::string current_loaded_file_path = "";					//現在エディタで開いているファイルのパス名
	uint32_t target_model_hash = 0;								//対象モデルのハッシュ値
	uint32_t runtime_active_node_id = UINT32_MAX;				//実行中のステートID
};