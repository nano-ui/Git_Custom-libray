#pragma once
#include "DirectXMath.h"

class RayUtility
{
public:
	//クリック座標を算出
	static bool ScreenToWorldRay(
			float window_height,
			float window_width,
			DirectX::XMFLOAT2 click_pos,
			DirectX::XMFLOAT4X4 view,
			DirectX::XMFLOAT4X4 projection,
			DirectX::XMFLOAT3& out_ray_start,
			DirectX::XMFLOAT3& out_ray_end
		);

	//NDC座標からレイを算出
	static bool NdcToWorldRay(
		DirectX::XMFLOAT2 ndc,
		DirectX::XMFLOAT4X4 view,
		DirectX::XMFLOAT4X4 projection,
		DirectX::XMFLOAT3& out_ray_start,
		DirectX::XMFLOAT3& out_ray_end
	);
};