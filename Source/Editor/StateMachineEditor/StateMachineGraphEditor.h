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
class StateGraphConfigManager;
class StateMachineComponent;
class AssetLoader;
class StateBlackboardInspectorWindow;
class StateNodeRenderer;
class StateLinkRenderer;
class StateGraphNavigator;
class StateCanvasInteractionHandler;
class StateLinkConnectionHandler;
class StateGraphCameraController;
class StateGraphToolbar;
class StateGraphSimulator;

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
	//追従ロジックとタイマー更新
	void UpdateRuntimeTracking();

	//擬似シミュレーション更新
	void UpdateSimulationMode(StateBlackboard* blackboard, GraphData* current_graph, uint32_t& current_active_node_id);

	//アクティブノードのアニメーション同期
	void SyncActiveNodeAnimation(GraphData* current_graph, uint32_t active_node_id); 

	//左パレットとノードリスト
	void DrawLeftSidebar(GraphData* current_graph, float width, float height);

	//メインのノードエディタキャンバス
	void DrawCenterCanvas(GraphData* current_graph, float width, float height);

	//右プロパティウインドウ
	void DrawRightSidebar(GraphData* current_graph, StateBlackboard* blackboard, float width, float height);

	//アニメーションマップを構築して送信
	void TriggerHotReload();

private:
	//カスタムデリータ
	struct EditorContexDeleter
	{
		void operator()(ax::NodeEditor::EditorContext* context)const noexcept;
	};

private:
	std::unique_ptr<StateGraphDataManager> data_manager;			//データを専門的に扱うマネージャー
	std::unique_ptr<StateGraphPaletteWindow> palette_window;		//左ペイン：パレット描画クラス
	std::unique_ptr<StateGraphPropertyWindow> property_window;		//右ペイン：プロパティ描画クラス
	std::unique_ptr<StateNodeRenderer> state_node_renderer;			//ステートノード描画クラス
	std::unique_ptr<StateLinkRenderer> state_link_renderer;			//ステートリンク描画クラス
	std::unique_ptr<StateGraphNavigator> state_graph_navigator;		//階層ナビゲーションバーの描画およびサブグラフへの潜り込み判定クラス
	std::unique_ptr<StateCanvasInteractionHandler> canvas_interaction_handler;	//キャンバス操作・ユーザー入力ハンドラー
	std::unique_ptr<StateLinkConnectionHandler> link_connection_handler;	//リンク接続・検証ハンドラー
	std::unique_ptr<StateGraphCameraController> camera_controller;          //カメラ・ビューポート制御クラス
	std::unique_ptr<StateGraphToolbar> toolbar;								//上部ツールバー及びファイル・アセット操作クラス
	std::unique_ptr<StateGraphSimulator> state_graph_simulator;				//ステートマシン共通シミュレータ

	std::unique_ptr<ax::NodeEditor::EditorContext, EditorContexDeleter> editor_context;	//エディタのライフサイクルを管理
	std::unique_ptr<StateGraphConfigManager> config_manager;
	std::unique_ptr<AssetLoader> asset_loader;								//モデル読み込みクラス
	std::unique_ptr<StateBlackboard> editor_dummy_blackboard;				//未選択時に使用するエディタ専用のダミーブラックボード
	std::unique_ptr<StateBlackboardInspectorWindow> blackboard_inspector;	//シミュレーション用パラメータ表示ウィンドウ
	std::unique_ptr<StateMachineComponent> state_machine_component;			//実機互換のシミュレーション実行コンポーネント

	uint32_t current_graph_id;									//現在の階層のグラフID
	std::unordered_map<uint32_t, uint32_t> graph_active_nodes;	//各階層ごとのアクティブノードIDを個別に保持
	uint32_t current_active_node_id = 0;						//現在のアクティブノードID
	std::string current_loaded_file_path = "";					//現在エディタで開いているファイルのパス名
	uint32_t target_model_hash = 0;								//対象モデルのハッシュ値
	uint32_t runtime_active_node_id = UINT32_MAX;				//実行中のステートID

	uint32_t flow_src_node_id = 0;								//遷移エフェクトの発生元となったノードID
	uint32_t flow_dst_node_id = 0;								//遷移エフェクトの遷移先となったノードID
	float flow_effect_timer = 0.0f;								//遷移エフェクトの残り表示時間（秒）
	bool is_tracking_active_node = false;						//実行中のアクティブノードを自動で追尾する状態フラグ
	bool is_simulation_active = false;							//エディタ上での擬似シミュレーション実行フラグ
	uint32_t last_tracked_runtime_node_id = UINT32_MAX;			//直前に追尾処理を行ったゲーム側のアクティブノードID
	uint32_t last_synced_node_id = UINT32_MAX;					//直前に同期を行ったノードID
	bool has_flow_requsted = false;								//エフェクトリクエスト
};