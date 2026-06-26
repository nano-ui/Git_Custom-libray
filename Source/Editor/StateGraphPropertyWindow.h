#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

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

private:
	std::unique_ptr<TransitionConditionEditor> condition_editor;	//条件遷移UI
};

