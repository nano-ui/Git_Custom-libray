#include "GraphNode.h"

#include <Windows.h>

//コンストラクタ
GraphNode::GraphNode()
{
	node_basic_data.id = UINT32_MAX;
	node_basic_data.name = "";
	node_basic_data.position = { 0.0f,0.0f };
	node_basic_data.is_sub_graph = false;
	node_basic_data.sub_graph_id = UINT32_MAX;
	node_basic_data.node_type = GraphNodeType::None;

	is_initialized = false;
	input_pins = {};
	output_pins = {};
	next_pin_id = 0;
}

//初期化処理
void GraphNode::Initialize(const NodeBasicData& basic_data)
{
	if (is_initialized)
	{
		OutputDebugStringA("[GraphNode - Initialize] 初期化処理が2重呼び出しされています");
		return;
	}

	node_basic_data = basic_data;
	SetupPins();

	is_initialized = true;
}

//入力ピン追加
void GraphNode::AddInputPin(const std::string& pin_name)
{
	PinData new_pin_data;
	new_pin_data.pin_name = pin_name;
	new_pin_data.pin_type = PinType::Input;
	new_pin_data.pin_id = next_pin_id;

	input_pins.push_back(new_pin_data);
	next_pin_id++;
}

//出力ピン追加
void GraphNode::AddOutputPin(const std::string& pin_name)
{
	PinData new_pin_data;
	new_pin_data.pin_name = pin_name;
	new_pin_data.pin_type = PinType::Output;
	new_pin_data.pin_id = next_pin_id;

	output_pins.push_back(new_pin_data);
	next_pin_id++;
}

//指定されたピンを検索
bool GraphNode::HasPin(uint32_t pin_id) const
{
	for (size_t n = 0; n < input_pins.size(); n++)
	{
		if (pin_id == input_pins[n].pin_id)
		{
			return true;
		}
	}

	for (size_t n = 0; n < output_pins.size(); n++)
	{
		if (pin_id == output_pins[n].pin_id)
		{
			return true;
		}
	}

	return false;
}
