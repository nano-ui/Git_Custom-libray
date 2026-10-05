#pragma once

#include <memory>
#include <string>

class StateBlackboard;
struct GraphData;

class StateBlackboardInspectorWindow
{
public:
	//コンストラクタ
	StateBlackboardInspectorWindow() = default;

	//デストラクタ
	~StateBlackboardInspectorWindow() = default;

	//遷移条件変数を登録
	void SyncBlackboardVariablesFromGraph(const GraphData* current_graph, StateBlackboard* blackboard);

	//ImGui描画
	void DrawInspector(StateBlackboard* blackboard);

private:
	//変数の型に応じたImGui描画
	void DrawVariableControl(StateBlackboard* blackboard, const std::string& var_name, uint32_t hash_key);
};

