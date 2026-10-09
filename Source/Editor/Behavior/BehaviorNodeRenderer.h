#pragma once

struct GraphNode;

class BehaviorNodeRenderer
{
public:
	//コンストラクタ
	BehaviorNodeRenderer() = default;

	//デストラクタ
	~BehaviorNodeRenderer() = default;

	//ノード描画
	void DrawNode(const GraphNode& node, bool is_active);

private:
	//入力ピン描画
	void DrawInputPins(const GraphNode& node);

	//出力ピン描画
	void DrawOutputPins(const GraphNode& node);
};

