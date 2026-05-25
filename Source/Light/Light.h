#pragma once

#include "DirectXMath.h"

class Light
{
public:
	//コンストラクタ
	Light();

	//ライトの方向ベクトルを取得
	DirectX::XMFLOAT4 GetDirection()const { return light_direction; }

	//ライトの方向ベクトルを設定
	void SetDirection(const DirectX::XMFLOAT4& direction) { light_direction = direction; }

private:
	DirectX::XMFLOAT4 light_direction;	//平行光源の向き
};

