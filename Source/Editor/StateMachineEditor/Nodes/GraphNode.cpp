#include "GraphNode.h"
#include "Serialization\JsonSerializer.h"

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

//保存変数登録処理
void GraphNode::SetupSerializer(JsonSerializer* serializer)
{
	if (!serializer)return;
	serializer->RegisterVariable(u8"ノードID", &node_basic_data.id);
	serializer->RegisterVariable(u8"ノード名", &node_basic_data.name);
	serializer->RegisterVariable(u8"座標", &node_basic_data.position);
	serializer->RegisterVariable(u8"サブグラフフラグ", &node_basic_data.is_sub_graph);
	serializer->RegisterVariable(u8"サブグラフID", &node_basic_data.sub_graph_id);
	serializer->RegisterVariable(u8"ノード属性", &node_basic_data.node_type);
	serializer->RegisterVariable(u8"次のピンID", &next_pin_id);
	serializer->RegisterVector(u8"入力ピン", &input_pins);
	serializer->RegisterVector(u8"出力ピン", &output_pins);
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

//ピンの属性を取得
PinType GraphNode::GetPinType(uint32_t pin_id) const
{
	for (size_t n = 0; n < input_pins.size(); n++)
	{
		if (pin_id == input_pins[n].pin_id)
		{
			return input_pins[n].pin_type;
		}
	}

	for (size_t n = 0; n < output_pins.size(); n++)
	{
		if (pin_id == output_pins[n].pin_id)
		{
			return output_pins[n].pin_type;
		}
	}
	return PinType::None;
}
