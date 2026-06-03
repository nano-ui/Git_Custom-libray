#pragma once

#include <DirectXMath.h>

//======================
//回転の基底クラス
//======================
class RotationBase
{
public:
	virtual ~RotationBase() = default;

	//更新処理
	virtual DirectX::XMVECTOR Update(const DirectX::XMVECTOR& current_ratation, const DirectX::XMFLOAT3& target_dir) = 0;
};

