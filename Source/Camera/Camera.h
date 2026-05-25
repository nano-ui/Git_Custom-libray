#pragma once

#include "DirectXMath.h"

class Camera
{
public:
	//コンストラクタ
	Camera();

	//ビュープロジェクション行列を計算
	void CalculateViewProjection(float aspect_ratio);

	//ビュープロジェクション行列を取得
	DirectX::XMFLOAT4X4 GetViewProjection()const { return view_projection; }

	//カメラ座標を取得
	DirectX::XMFLOAT3 GetPosition()const { return camera_position; }

	//カメラ座標を設定
	void SetPosition(const DirectX::XMFLOAT3& position) { camera_position = position; }

private:
	DirectX::XMFLOAT3 camera_position;		//カメラ座標
	DirectX::XMFLOAT4X4 view_projection;	//計算結果の行列を保持
};

