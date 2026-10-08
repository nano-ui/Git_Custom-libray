#pragma once

#include "DirectXMath.h"
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "ThiedParty\json.hpp"
#include "GraphDataManager.h"

class StateGraphNode;

class StateGraphDataManager : public GraphDataManager
{
public:
	//コンストラクタ
	StateGraphDataManager();

	//デストラクタ
	~StateGraphDataManager() = default;

	//ファイルに保存
	void SaveToFile(const std::string& file_path);

	//ファイル読み込み
	bool LoadFromFile(const std::string& file_path);

	//ステートノード追加
	uint32_t AddStateNode(uint32_t graph_id, DirectX::XMFLOAT2 click, const std::string name = u8"新規ステート");

	//サブグラフノードの生成
	void AddSubGrapNode(uint32_t graph_id, float click_x, float click_y, const std::string& name = u8"新規サブグラフ");

	//既存のノードをサブグラフに変換
	void ConvertToSubGraph(uint32_t graph_id, uint32_t node_id);

	//階層が空の場合に初期ノードを構築
	void CheckAndInitDefaultNode(uint32_t graph_id);

	//遷移条件を追加
	void AddConditionToLink(uint32_t graph_id, uint32_t link_id);

	//遷移条件を削除
	void DeleteConditionFromLink(uint32_t graph_id, uint32_t link_id, size_t condition_index);

	//全ての階層リストを取得
	std::vector<GraphData>& GetGraphDatas() { return graph_datas; }

	//モデルパスを取得
	const std::string& GetTargetModelPath()const { return target_model_path; }

	//モデルパスの設定
	void SetTargetModelPath(const std::string& path) { target_model_path = path; }

private:
	std::string target_model_path = "";	//紐づけ対象のパス
};

