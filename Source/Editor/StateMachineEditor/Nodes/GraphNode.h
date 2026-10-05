#pragma once
#include "GraphNodeData.h"

#include <DirectXMath.h>
#include <string>
#include <vector>


class GraphNode
{
public:
	//コンストラクタ
	GraphNode();

	//デストラクタ
	virtual  ~GraphNode() = default;

	//初期化処理
	void Initialize(const NodeBasicData& basic_data);

	//ピンの登録処理
	virtual void SetupPins() = 0;

	//入力ピン追加
	void AddInputPin(const std::string& pin_name);

	//出力ピン追加
	void AddOutputPin(const std::string& pin_name);


private:
	NodeBasicData node_basic_data;		//基本ノード情報
	bool is_initialized;				//初期化フラグ
	uint32_t next_pin_id;				//次のピンID
	std::vector<PinData> input_pins;	//入力ピン配列
	std::vector<PinData> output_pins;	//出力ピン配列
};

