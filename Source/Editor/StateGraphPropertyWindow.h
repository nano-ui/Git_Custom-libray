#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

class StateGraphDataManager;
class StateBlackboard;
class TransitionConditionEditor;

struct GraphData;
struct GraphTransitionCondition;

class StateGraphPropertyWindow
{
public:
	//コンストラクタ
	StateGraphPropertyWindow();

	//デストラクタ
	~StateGraphPropertyWindow();

	//プロパティウィンドウの全体描画
	bool DrawProperty(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		StateBlackboard* blackboard,
		const std::vector<std::string>& anim_names);

private:
	//ノード選択時の詳細プロパティ描画
	bool DrawNodeProperty(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		uint32_t node_id,
		StateBlackboard* blackboard,
		const std::vector<std::string>& anim_names);

	//リンク選択時の詳細プロパティ
	bool DrawLinkProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t node_id, StateBlackboard* blackboard);

	//入力チェック条件専用のImGui入力UI描画
	void DrawInputCompareUI(GraphTransitionCondition& conditon);

private:
	std::unique_ptr<TransitionConditionEditor> condition_editor;	//条件遷移UI
	GraphTransitionCondition* waiting_for_key_conditon = nullptr;	//入力条件のポインタ
};

