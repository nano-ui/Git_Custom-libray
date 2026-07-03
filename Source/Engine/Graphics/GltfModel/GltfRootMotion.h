#pragma once

#include "DirectXMath.h"
#include <memory>
#include <string>

class GltfModelData;

class GltfRootMotion
{
public:
	//初期化
	void Initialize(const std::shared_ptr<const GltfModelData>& data, const std::string& root_node_name = "B_Pelvis");

	//アニメーションの進行に合わせてルートノードの差分を計算
	void Update(size_t animation_index, float current_time);

	//計算された位置の差分を取得
	DirectX::XMFLOAT3 GetDeltaPosition()const;

	//計算された回転の差分を取得
	DirectX::XMFLOAT4 GetDeltaRotation()const;

	//対象のノードを取得
	int GetTargetNodeIndex()const { return target_node_index; }

	//アニメーションの切り替えやループ時に、前回の状態をリセット
	void ResetDelta();

private:
	//任意の時間のルートノードの位置と回転を計算
	void ComputeRootPose(size_t animation_index, float time, DirectX::XMFLOAT3& out_position, DirectX::XMFLOAT4& out_rotation)const;

	//階層構造からルートモーションの対象となるノードインデックスを検索
	int FindRootNodeIndex(const std::string& root_node_name)const;

private:
	std::shared_ptr<const GltfModelData> model_data;				//参照するモデル情報
	int target_node_index = 0;										//ルートとして計算するノードの番号
	DirectX::XMFLOAT3 previous_position = { 0.0f,0.0f,0.0f };		//前回のルートノードの位置
	DirectX::XMFLOAT4 previous_rotation = { 0.0f,0.0f,0.0f,1.0f };	//前回のルートノードの回転
	DirectX::XMFLOAT3 delta_position = { 0.0f,0.0f,0.0f };			//位置の差分
	DirectX::XMFLOAT4 delta_rotation = { 0.0f,0.0f,0.0f,1.0f };		//回転の差分
	bool is_first_update = true;									//最初の更新化の判定フラグ
	float previous_time = 0.0f;										//前回の再生時間
};

