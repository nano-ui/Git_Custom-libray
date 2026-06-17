#pragma once
#include "MoveBase.h"
class RepulsiveMove : public MoveBase
{
public:
	//コンストラクタ
	RepulsiveMove(uint32_t power_k, uint32_t radius_k);

	//デストラクタ
	virtual ~RepulsiveMove() = default;

	//更新処理
	DirectX::XMFLOAT3 Update(
		const DirectX::XMFLOAT3& current_pos,
		const DirectX::XMFLOAT3& target_pos,
		const StateBlackboard* blackboard)override;

private:

	//斥力更新処理
	DirectX::XMFLOAT3 UpdateRepulsion(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, const float repulsion_power, const float effect_radius);

private:
	uint32_t repulsion_power_key;	//最大反発スピード取得用のハッシュキー
	uint32_t effect_radius_key;		//斥力影響範囲取得のハッシュキー
};

