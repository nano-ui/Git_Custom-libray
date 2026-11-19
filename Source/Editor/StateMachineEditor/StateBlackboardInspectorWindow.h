#pragma once

#include <memory>
#include <string>

class Blackboard;
struct GraphData;

//ステートマシンのブラックボード変数を監視・編集するインスペクターウィンドウ
class BlackboardInspectorWindow
{
public:
	//コンストラクタ
	BlackboardInspectorWindow() = default;

	//デストラクタ
	~BlackboardInspectorWindow() = default;

	//グラフ内の遷移条件変数がブラックボードに登録済みか検証
	void SyncBlackboardVariablesFromGraph(const GraphData* current_graph, Blackboard* blackboard);

	//インスペクターウィンドウのImGui描画
	void DrawInspector(Blackboard* blackboard);

private:
	//変数の型に応じたImGui操作UIの描画
	void DrawVariableControl(Blackboard* blackboard, const std::string& var_name);
};