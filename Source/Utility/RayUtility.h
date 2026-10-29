#pragma once
#include "DirectXMath.h"

//クリック座標を算出
bool ScreenToWorldRay(
	float window_height,
	float window_width,
	DirectX::XMFLOAT2 click_pos,
	DirectX::XMFLOAT4X4 view,
	DirectX::XMFLOAT4X4 projection,
	DirectX::XMFLOAT3& out_ray_start,
	DirectX::XMFLOAT3& out_ray_end
);