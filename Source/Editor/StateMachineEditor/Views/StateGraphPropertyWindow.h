#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

class StateGraphDataManager;
class StateBlackboard;
class TransitionConditionEditor;
class StateGraphNode;
struct GraphData;
struct GraphTransitionCondition;

class StateGraphPropertyWindow
{
public:
	// コンストラクタ
	StateGraphPropertyWindow();
	// デストラクタ
	~StateGraphPropertyWindow();

	// プロパティウィンドウ描画
	bool DrawProperty(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		StateBlackboard* blackboard,
		const std::vector<std::string>& anim_names);

private:
	// ノードプロパティ描画
	bool DrawNodeProperty(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		uint32_t node_id,
		StateBlackboard* blackboard,
		const std::vector<std::string>& anim_names);

	// ノードのアクション・アニメーション設定UI
	bool DeawNodeActionSettings(StateGraphNode* target_node, const std::vector<std::string>& anim_names);

	// ノードからの遷移リンク設定UI
	bool DrawNodeTransitionSettings(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		StateGraphNode* target_node,
		StateBlackboard* blackboard);

	// リンクプロパティ描画
	bool DrawLinkProperty(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		uint32_t link_id,
		StateBlackboard* blackboard);

	// 入力比較用UI
	void DrawInputCompareUI(GraphTransitionCondition& conditon);

private:
	std::unique_ptr<TransitionConditionEditor> condition_editor;
	GraphTransitionCondition* waiting_for_key_conditon = nullptr;
	int selected_output_link_index = -1;
};