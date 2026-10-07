#pragma once
#include "ThiedParty\json.hpp"

#include <DirectXMath.h>
#include <string>
#include <vector>

//ノード種別
enum class GraphNodeType
{
	None,					//未設定
	StateNode,				//ステートマシン用ノード
	BehaviorRootNode,		//ツリー基底ノード
	BehaviorCompositeNode,	//ツリー中間制御ノード
	BehaviorActionNode		//ツリー末端行動ノード
};

//ピン種別
enum class PinType
{
	None,	//未設定
	Input,	//入力
	Output	//出力
};

//ノード基本情報
struct NodeBasicData
{
	uint32_t id;				//識別ID
	std::string name;			//ノード名
	DirectX::XMFLOAT2 position;	//座標
	bool is_sub_graph;			//サブグラフフラグ
	uint32_t sub_graph_id;		//サブグラフ識別ID
	GraphNodeType node_type;	//ノード属性
};

//ピン情報
struct PinData
{
	uint32_t pin_id;		//ピン識別ID
	std::string pin_name;	//ピン名
	PinType pin_type;		//ピン属性
};

namespace nlohmann
{
	inline void to_json(json& json_data, const GraphNodeType& node_type)
	{
		json_data = static_cast<int>(node_type);
	}

	inline void from_json(const json& json_data, GraphNodeType& node_type)
	{
		int type = json_data.get<int>();
		node_type = static_cast<GraphNodeType>(type);
	}

	inline void to_json(json& json_data, const PinType& pin_type)
	{
		json_data = static_cast<int>(pin_type);
	}

	inline void from_json(const json& json_data, PinType& pin_type)
	{
		int type = json_data.get<int>();
		pin_type = static_cast<PinType>(type);
	}

	inline void to_json(json& json_data, const PinData& pin_data)
	{
		json_data = json
		{
			{u8"ピンID",pin_data.pin_id},
			{u8"ピン名",pin_data.pin_name },
			{u8"ピン属性",pin_data.pin_type}
		};
	}

	inline void from_json(const json& json_data, PinData& pin_data)
	{
		json_data.at(u8"ピンID").get_to(pin_data.pin_id);
		json_data.at(u8"ピン名").get_to(pin_data.pin_name);
		json_data.at(u8"ピン属性").get_to(pin_data.pin_type);
	}
}