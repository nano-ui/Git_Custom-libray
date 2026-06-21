#pragma once

#include <cstdint>

class StateGraphDataManager;
class StateBlackboard;
struct GraphData;
struct GraphLink;
struct GraphTransitionCondition;

class TransitionConditionEditor
{
public:
	//コンストラクタ
	TransitionConditionEditor();

	//デストラクタ
	~TransitionConditionEditor();

	//リンクの遷移条件設定を描画
	void DrawConditonSettings(StateGraphDataManager* data_manager, StateBlackboard* blackboard, uint32_t grap_id, GraphLink* target_link);

private:
	//通常比較用のImGui入力UI描画
	void DrawNormalCompareUI(StateBlackboard* blackboard, GraphTransitionCondition& condition);

	//確率判定用のImGui入力UI描画
	void DrawRandomUI(GraphTransitionCondition& condition);

	//距離判定用のImGui入力UI描画
	void DrawDistanceUI(StateBlackboard* blackboard, GraphTransitionCondition& condition);

	//割合判定用のImGui入力UI描画
	void DrawRatioUI(StateBlackboard* blackboard, GraphTransitionCondition& condition);
};

