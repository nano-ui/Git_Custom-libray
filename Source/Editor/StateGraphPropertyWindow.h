#pragma once

#include <cstdint>
#include <memory>

class StateGraphDataManager;
class StateBlackboard;
class TransitionConditionEditor;
struct GraphData;

class StateGraphPropertyWindow
{
public:
	//コンストラクタ
	StateGraphPropertyWindow();

	//デストラクタ
	~StateGraphPropertyWindow();

	//プロパティウィンドウの全体描画
	bool DrawProperty(StateGraphDataManager* data_manager, GraphData* current_graph, StateBlackboard* blackboard);

private:
	//ノード選択時の詳細プロパティ描画
	bool DrawNodeProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t node_id, StateBlackboard* blackboard);

	//リンク選択時の詳細プロパティ
	void DrawLinkProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t node_id, StateBlackboard* blackboard);

private:
	std::unique_ptr<TransitionConditionEditor> condition_editor;	//条件遷移UI
};

