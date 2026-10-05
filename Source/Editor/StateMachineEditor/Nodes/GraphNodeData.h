#pragma once

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
