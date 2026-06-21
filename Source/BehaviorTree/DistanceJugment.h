#pragma once
#include "JudgmentNode.h"

#include <memory>
#include <functional>
#include <DirectXMath.h>

class DistanceJudgment :public JudgmentNode
{
public:
	DistanceJudgment(
		std::reference_wrapper<const DirectX::XMFLOAT3> my_pos,
		std::reference_wrapper<const DirectX::XMFLOAT3> target_pos,
		float min_dist,
		float max_dist
	);

	bool Check()override;

private:
	std::reference_wrapper<const DirectX::XMFLOAT3> my_position;	 //自身の位置への参照
	std::reference_wrapper<const DirectX::XMFLOAT3> target_position; //対象の位置への参照
	float min_threshold;	//近づくときの閾値
	float max_threshold;	//離れるときの閾値
};

