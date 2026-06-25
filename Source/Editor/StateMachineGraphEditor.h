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

	uint32_t current_graph_id;									//現在の階層のグラフID
	std::unordered_map<uint32_t, uint32_t> graph_active_nodes;	//各階層ごとのアクティブノードIDを個別に保持
	uint32_t previous_active_node_id = 0;						//前フレームのアクティブノードID
	uint32_t current_active_node_id = 0;						//現在のアクティブノードID
	uint32_t auto_flowing_link_id = 0;							//アニメーションを実行するリンクID
	float auto_flow_timer = 0.0f;								//エフェクトの有効時間
	std::string current_loaded_file_path = "";					//現在エディタで開いているファイルのパス名
};