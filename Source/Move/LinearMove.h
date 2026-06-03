#pragma once
#include "MoveBase.h"
class LinearMove : public MoveBase
{
public:
	//更新処理
	DirectX::XMFLOAT3 Update(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos) { return { 0.0f,0.0f,0.0f }; }

	//直線移動更新処理
	DirectX::XMFLOAT3 UpdateLiner(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, const float speed);
};

