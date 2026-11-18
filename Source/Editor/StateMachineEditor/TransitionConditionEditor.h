#pragma once

#include <cstdint>

class StateGraphDataManager;
class Blackboard;
struct GraphData;
struct GraphLink;
struct TransitionCondition;

class TransitionConditionEditor
{
public:
	//コンストラクタ
	TransitionConditionEditor();

	//デストラクタ
	~TransitionConditionEditor();

	//リンクの遷移条件設定を描画
	bool DrawConditonSettings(StateGraphDataManager* data_manager, Blackboard* blackboard, uint32_t grap_id, GraphLink* target_link);

private:
	//通常比較用のImGui入力UI描画
	bool DrawNormalCompareUI(Blackboard* blackboard, TransitionCondition& condition);

	//確率判定用のImGui入力UI描画
	void DrawRandomUI(TransitionCondition& condition);

	//距離判定用のImGui入力UI描画
	void DrawDistanceUI(Blackboard* blackboard, TransitionCondition& condition);

	//割合判定用のImGui入力UI描画
	void DrawRatioUI(Blackboard* blackboard, TransitionCondition& condition);

	//キーボードの入力を設定するUI描画
	void DrawInputCheckUI(TransitionCondition& condition);

private:
	TransitionCondition* waiting_for_key_condition = nullptr;	//現在キーボードの入力を待機している条件のポインタ
};

