#include "Camera.h"

//=================
//コンストラクタ
//=================
Camera::Camera()
{
	camera_position = { 0.0f,0.0f,10.0f };
	DirectX::XMStoreFloat4x4(&view_projection, DirectX::XMMatrixIdentity());
}

//==================================
//ビュープロジェクション行列を計算
//==================================
void Camera::CalculateViewProjection(float aspect_ratio)
{
	//----------------------------------------
	//ビュー行列とプロジェクション行列の計算
	//----------------------------------------
	DirectX::XMVECTOR eye = DirectX::XMVectorSet(camera_position.x, camera_position.y, camera_position.z, 1.0f);
	DirectX::XMVECTOR focus = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMMATRIX view_matrix = DirectX::XMMatrixLookAtLH(eye, focus, up);
	DirectX::XMMATRIX projection_matrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(30.0f), aspect_ratio, 0.1f, 100.0f);
	DirectX::XMStoreFloat4x4(&view_projection, view_matrix * projection_matrix);
}
