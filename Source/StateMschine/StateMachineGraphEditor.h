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
	void DrawHeaderNavigation();

	//ステート一覧リストを描画して、その位置に移動
	void DrawStateListWindow(GraphData* current_graph);

	//ノードの削除
	void DeleteNode(GraphData* current_graph);

	//接続線の削除
	void DeleteLink(GraphData* current_graph);

	//ノードの詳細情報を描画
	void DrawPropertyWindow(GraphData* current_graph);

	//接続線の作成を検知してデータに追加
	void CreateNewLink(GraphData* current_graph);

	//リンクの接続ルールを判定
	bool CheckCanConnect(GraphData* current_graph, uint32_t start_id, uint32_t end_id);

	//遷移条件を構築
	void OnLinkCreated(GraphData* current_graph, const GraphLink& new_link);

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
};

