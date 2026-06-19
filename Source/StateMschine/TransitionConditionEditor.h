#pragma once

#include <cstdint>

class StateGraphDataManager;
class StateBlackboard;
struct GraphData;
struct GraphLink;

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

};

