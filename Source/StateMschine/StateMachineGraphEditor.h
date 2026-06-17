#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

class StateMachine;
class StateBlackboard;

//ピンの種類
enum class PinKind
{
	Input,	//入力
	Output	//出力
};

//ノードの端子の情報
struct GraphPin
{
	uint32_t id;		//固有ID
	std::string name;	//ピン名
	PinKind kind;		//ピンの種類
	uint32_t node_id;	//所属しているノードID
};

//ステート情報
struct GraphNode
{
	uint32_t id;					//ノードの固有ID
	std::string name;				//ステート名
	float position_x;				//X座標
	float position_y;				//Y座標
	std::vector<GraphPin> input;	//入力ピンのリスト
	std::vector<GraphPin> output;	//出力ピンのリスト
	bool is_sub_graph;				//階層型ステートマシンのグラフ
	uint32_t sub_graph_id;			//下位階層のグラフID
};

//ノードを繋ぐ線の情報
struct GraphLink
{
	uint32_t id;			//線の固有ID
	uint32_t start_pin_id;	//接続元の出力ピンID
	uint32_t end_pin_id;	//接続先の入力ピンID
};

//階層の情報
struct GraphData
{
	uint32_t id;					//グラフのID
	std::string name;				//階層名
	std::vector<GraphNode> nodes;	//階層に存在するノード群
	std::vector<GraphLink> links;	//階層に存在する接続線群
};

class StateMachineGraphEditor
{
public:
	//コンストラクタ
	StateMachineGraphEditor();

	//エディタ描画
	void DrawEditor(StateBlackboard* blackboard);

private:
	std::vector<GraphData> layer_datas;	//全ての階層データのリスト
	uint32_t current_graph_id;			//現在の階層のグラフID
};

