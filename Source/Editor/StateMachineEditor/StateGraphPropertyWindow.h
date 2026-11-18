#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

class StateGraphDataManager;
class Blackboard;
class TransitionConditionEditor;

struct GraphData;
struct GraphNode;
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
		Blackboard* blackboard,
		const std::vector<std::string>& anim_names);

private:
	//ノード選択時の詳細プロパティ描画
	bool DrawNodeProperty(
		StateGraphDataManager* data_manager,
		GraphData* current_graph,
		uint32_t node_id,
		Blackboard* blackboard,
		const std::vector<std::string>& anim_names);

	//ノードのアクションとアニメーション設定に関するUI描画
	bool DeawNodeActionSettings(GraphNode* target_node, const std::vector<std::string>& anim_names);

	//ノードから出発する遷移線とその条件に関するUI描画
	bool DrawNodeTransitionSettings(StateGraphDataManager* data_manager, GraphData* current_graph, GraphNode* target_node, Blackboard* blackboard);

	//リンク選択時の詳細プロパティ
	bool DrawLinkProperty(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t node_id, Blackboard* blackboard);

	//入力チェック条件専用のImGui入力UI描画
	void DrawInputCompareUI(GraphTransitionCondition& conditon);

	//ビヘイビアツリーノード用のプロパティ描画
	bool DrawBehaviorNodeProperty(GraphNode* target_node, const std::vector<std::string>& anim_names);

private:
	std::unique_ptr<TransitionConditionEditor> condition_editor;	//条件遷移UI
	GraphTransitionCondition* waiting_for_key_conditon = nullptr;	//入力条件のポインタ
	int selected_output_link_index = -1;	//現在ノードプロパティ内で選択されている出発リンクのインデックス
};

