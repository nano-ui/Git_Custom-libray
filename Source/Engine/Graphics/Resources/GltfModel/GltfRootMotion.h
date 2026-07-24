#pragma once

#include "DirectXMath.h"
#include <memory>
#include <string>
#include <unordered_map>

class GltfModelData;

class GltfRootMotion
{
public:
	//無効なノードインデックス
	static constexpr int INVALID_NODE_INDEX = -1;

public:
	//初期化
	void Initialize(const std::shared_ptr<const GltfModelData>& data);

	//アニメーションの進行に合わせてルートノードの差分を計算
	void Update(size_t animation_index, float current_time);

	//計算された位置の差分を取得
	DirectX::XMFLOAT3 GetDeltaPosition()const;

	//計算された回転の差分を取得
	DirectX::XMFLOAT4 GetDeltaRotation()const;

	//対象のノードを取得
	int GetTargetNodeIndex()const { return current_target_node_index; }

	//アニメーションの切り替えやループ時に、前回の状態をリセット
	void ResetDelta();

	//初期ポーズのローカル位置を取得
	DirectX::XMFLOAT3 GetInitialLocalPosition() const { return initial_local_position; }

private:
	//各アニメーションデータを走査して移動値のある子ノードを事前に自動検出
	void AnalyzeAnimations();

	//任意の時間のルートノードの位置と回転を計算
	void ComputeRootPose(size_t animation_index, float time, DirectX::XMFLOAT3& out_position, DirectX::XMFLOAT4& out_rotation)const;

	//階層構造からルートモーションの対象となるノードインデックスを検索
	int FindRootNodeIndex(const std::string& root_node_name)const;

private:
	std::shared_ptr<const GltfModelData> model_data;				//参照するモデル情報
	int base_root_node_index = INVALID_NODE_INDEX;					//最上位ルートノードインデックス
	int current_target_node_index = INVALID_NODE_INDEX;				//現在再生中のアニメーションにおいて実際に移動値を持っているノードのインデックス
	std::unordered_map<size_t, int> animation_target_nodes;			//移動値を持つノードインデックスのキャッシュマップ
	DirectX::XMFLOAT3 previous_position = { 0.0f,0.0f,0.0f };		//前回のルートノードの位置
	DirectX::XMFLOAT4 previous_rotation = { 0.0f,0.0f,0.0f,1.0f };	//前回のルートノードの回転
	DirectX::XMFLOAT3 delta_position = { 0.0f,0.0f,0.0f };			//位置の差分
	DirectX::XMFLOAT4 delta_rotation = { 0.0f,0.0f,0.0f,1.0f };		//回転の差分
	DirectX::XMFLOAT3 initial_local_position = { 0.0f, 0.0f, 0.0f };//現在のアニメーションにおける0秒時点のローカル位置
	bool is_first_update = true;									//最初の更新化の判定フラグ
	float previous_time = 0.0f;										//前回の再生時間
};

