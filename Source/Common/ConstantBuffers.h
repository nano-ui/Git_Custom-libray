#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>


struct SceneConstants
{
	DirectX::XMFLOAT4X4 view_projection;//ビュー・プロジェクション変換行列
	DirectX::XMFLOAT4 light_projection;	//ライトの向き
	DirectX::XMFLOAT4 camera_position;	//カメラの位置
};


class ConstantBuffers
{
};

