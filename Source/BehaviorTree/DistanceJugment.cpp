#include "DistanceJugment.h"

DistanceJudgment::DistanceJudgment(std::reference_wrapper<const DirectX::XMFLOAT3> my_pos, std::reference_wrapper<const DirectX::XMFLOAT3> target_pos, float min_dist, float max_dist)
	: my_position(my_pos), target_position(target_pos), min_threshold(min_dist), max_threshold(max_dist)
{
}
bool DistanceJudgment::Check()
{
	DirectX::XMFLOAT3 current_pos = my_position.get();		//自身の位置を設定
	DirectX::XMFLOAT3 target_pos = target_position.get();	//対象の位置を設定

	//距離の2乗を計算
	float dist_sq = (current_pos.x - target_pos.x) * (current_pos.x - target_pos.x) + (current_pos.z - target_pos.z) * (current_pos.z - target_pos.z);

	float min_sq = min_threshold * min_threshold;
	float max_sq = max_threshold * max_threshold;

	return (dist_sq >= min_sq && dist_sq <= max_sq);
}
