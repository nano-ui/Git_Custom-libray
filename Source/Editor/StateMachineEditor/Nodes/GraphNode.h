#pragma once
#include "GraphNodeData.h"

#include <DirectXMath.h>
#include <string>
#include <vector>

class JsonSerializer;

class GraphNode
{
public:
	//コンストラクタ
	GraphNode();

	//デストラクタ
	virtual  ~GraphNode() = default;

	//初期化処理
	void Initialize(const NodeBasicData& basic_data);

	//保存変数登録処理
	virtual void SetupSerializer(JsonSerializer* serializer);

	//ピンの登録処理
	virtual void SetupPins() = 0;

	//入力ピン追加
	void AddInputPin(const std::string& pin_name);

	//出力ピン追加
	void AddOutputPin(const std::string& pin_name);

	//指定されたピンを検索
	bool HasPin(uint32_t pin_id)const;

	//ピンの属性を取得
	PinType GetPinType(uint32_t pin_id)const;

	//ノード情報を取得
	const NodeBasicData GetNodeBasicData() const { return node_basic_data; }

	//ノード情報の設定
	void SetNodeBasicData(NodeBasicData basic_data) { node_basic_data = basic_data; }

	//入力ピン情報を取得
	const std::vector<PinData> GetInputPins()const { return input_pins; }

	//入力ピン情報の設定
	void SetInputPin(PinData input_pin) { input_pins.push_back(input_pin); }

	//出力ピン情報を取得
	const std::vector<PinData> GetOutputPins()const { return output_pins; }

	//出力ピン情報の設定
	void SetOutputPin(PinData output_pin) { output_pins.push_back(output_pin); }

private:
	NodeBasicData node_basic_data;		//基本ノード情報
	std::vector<PinData> input_pins;	//入力ピン配列
	std::vector<PinData> output_pins;	//出力ピン配列
	bool is_initialized;				//初期化フラグ
	uint32_t next_pin_id;				//次のピンID
};