#pragma once
#include <DirectXMath.h>

struct ObjectConstantBuffer
{
	DirectX::XMFLOAT4X4 world;					//ワールド変換行列
	DirectX::XMFLOAT4 material_color;			//マテリアル色
	DirectX::XMFLOAT4X4 bone_transformes[256];	//ボーン変換行列
};

struct SceneConstantBuffer
{
	DirectX::XMFLOAT4X4 view_projection;	//ビュープロジェクション行列
	DirectX::XMFLOAT4 light_direction;		//光源方向
	DirectX::XMFLOAT4 camera_position;		//カメラ位置
};