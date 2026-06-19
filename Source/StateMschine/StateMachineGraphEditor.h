#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <imgui.h>

class StateMachine;
class StateBlackboard;
class StateGraphDataManager;

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

	//ステート一覧リストを描画して、その位置に移動
	void DrawStateListWindow(GraphData* current_graph);

	//ノードの削除
	void DeleteNode(GraphData* current_graph);

	//接続線の削除
	void DeleteLink(GraphData* current_graph);

	//ノードの詳細情報を描画
	void DrawPropertyWindow(GraphData* current_graph);

	//リンク選択時の詳細プロパティ描画
	void DrawLinkProperty(GraphData* current_graph, uint32_t link_id);

	//ノード選択時の詳細プロパティ描画
	void DrawNodeProperty(GraphData* current_graph, uint32_t node_id);

	//接続線の作成を検知してデータに追加
	void CreateNewLink(GraphData* current_graph);

	//遷移条件を構築
	void OnLinkCreated(GraphData* current_graph, const GraphLink& new_link);

	//実在するノード名の一覧を全データから動的に集計して取得
	std::vector<std::string> GetExistingStateNames();

private:
	//カスタムデリータ
	struct EditorContexDeleter
	{
		void operator()(ax::NodeEditor::EditorContext* context)const noexcept;
	};

private:
	std::unique_ptr<StateGraphDataManager> data_manager;		//データを専門的に扱うマネージャー
	uint32_t current_graph_id;									//現在の階層のグラフID
	std::unique_ptr<ax::NodeEditor::EditorContext, EditorContexDeleter> editor_context;	//エディタのライフサイクルを管理
	std::vector<std::string> available_state_palette;	//追加可能な全てのステートリスト
	std::string pending_add_palette_node_name;			//パレットから追加予約されたステート名
};

