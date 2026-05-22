#pragma once

#include "GltfModelData.h"
#include <stack>
#include <vector>

class GltfModelAnimation
{
public:
	//親子関係をたどって各ノードのグローバル行列を計算
	void CumulateTransforms(const GltfModelData& data, std::vector<GltfModelData::node>& target_nodes); 

	//名前を指定してアニメーションの再生を開始
	void PlayAniamtion(const GltfModelData& data, const std::string& animation_name, bool is_loop);

	//アニメーション更新処理
	void UpdateAnimation(const GltfModelData& data, float delta_time, std::vector<GltfModelData::node>& animated_nodes);

private:
	//指定した時間のアニメーションを適用しノード情報を更新
	void Animate(const GltfModelData& data, size_t animation_index, float time, std::vector<GltfModelData::node>& animated_nodes);

	//行列計算用の再帰呼び出し
	void TraverseNodeForTransform(int node_index, std::vector<GltfModelData::node>& nodes, std::stack<DirectX::XMFLOAT4X4>& parent_global_transforms);

	//アニメーションのキーフレームを取得し補間係数を計算
	size_t GetAnimationKeyframeIndex(const std::vector<float>& timelines, float time, float& interpolation_factor);

	//アニメーションの全体の長さを計算
	float CalculateAnimationDuration(const GltfModelData& data, size_t animation_index);

private:
	size_t current_animation_index = 0;			//現在再生中のアニメーション番号
	float current_animation_time = 0.0f;		//現在の再生時間
	float current_animation_duration = 0.0f;	//アニメーション終了時間
	bool is_loop_enabled = true;				//ループ再生の有効フラグ
	bool is_playing = false;					//アニメーション再生中判定フラグ
};

