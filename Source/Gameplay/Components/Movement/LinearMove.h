#pragma once
#include "MoveBase.h"
class LinearMove : public MoveBase
{
public:
	//コンストラクタ
	LinearMove(uint32_t key);

	//更新処理
	DirectX::XMFLOAT3 Update(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, const StateBlackboard* blackboard)override;

	//直線移動更新処理
	DirectX::XMFLOAT3 UpdateLiner(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, const float speed);

private:
	uint32_t speed_key;
};

