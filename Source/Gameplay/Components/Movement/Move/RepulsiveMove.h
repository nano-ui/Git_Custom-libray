#pragma once
#include "MoveBase.h"
class RepulsiveMove : public MoveBase
{
public:
	//コンストラクタ
	RepulsiveMove();

	//デストラクタ
	virtual ~RepulsiveMove() = default;

	//更新処理
	DirectX::XMFLOAT3 Update(
		const DirectX::XMFLOAT3& current_pos,
		const DirectX::XMFLOAT3& target_pos,
		const Blackboard* blackboard)override;

private:

	//斥力更新処理
	DirectX::XMFLOAT3 UpdateRepulsion(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, const float repulsion_power, const float effect_radius);

private:
};

