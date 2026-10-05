#pragma once

struct GraphNode;

class StateNodeRenderer
{
public:
	StateNodeRenderer() = default;
	~StateNodeRenderer() = default;

	//単一ノードとその入力・出力ピンを描画
	void DrawNode(const GraphNode& node, bool is_active);

private:
	//入力ピン群を描画
	void DrawInputPins(const GraphNode& node);

	//出力ピン群を描画
	void DrawOutputPins(const GraphNode& node);
};

