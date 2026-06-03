#pragma once

#include <DirectXMath.h>

//======================
//移動の基底クラス
//======================
class MoveBase
{
public:
	virtual ~MoveBase() = default;

	//更新処理
	virtual DirectX::XMFLOAT3 Update(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos) = 0;
};

