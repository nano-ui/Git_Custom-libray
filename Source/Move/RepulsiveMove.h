#pragma once
#include "MoveBase.h"
class RepulsiveMove : public MoveBase
{
public:
	//更新処理
	DirectX::XMFLOAT3 Update(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos) { return{ 0.0f,0.0f,0.0f }; }

	//斥力更新処理
	DirectX::XMFLOAT3 UpdateRepulsion(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, const float repulsion_power, const float effect_radius);
};

