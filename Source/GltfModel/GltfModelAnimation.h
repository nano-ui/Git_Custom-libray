#pragma once

#include "GltfModelData.h"
#include <stack>
#include <vector>

class GltfModelAnimation
{
public:
	//親子関係をたどって各ノードのグローバル行列を計算
	void CumulateTransforms(const GltfModelData& data, std::vector<GltfModelData::node>& target_nodes); 

	//指定した時間のアニメーションを適用しノード情報を更新
	void Animate(const GltfModelData& data, size_t animation_index, float time, std::vector<GltfModelData::node>& animated_nodes);

private:
	//行列計算用の再帰呼び出し
	void TraverseNodeForTransform(int node_index, std::vector<GltfModelData::node>& nodes, std::stack<DirectX::XMFLOAT4X4>& parent_global_transforms);

	//アニメーションのキーフレームを取得し補間係数を計算
	size_t GetAnimationKeyframeIndex(const std::vector<float>& timelines, float time, float& interpolation_factor);
};

