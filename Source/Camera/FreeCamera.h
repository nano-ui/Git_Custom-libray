#pragma once

#include "Camera.h"

class FreeCamera : public Camera
{
public:
	//コンストラクタ
	FreeCamera();

	//デストラクタ
	~FreeCamera();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//ImGui描画処理
	void RenderGui()override;

private:
	DirectX::XMFLOAT2 rotation_angle;	//回転速度
	float move_speed;					//移動速度
	float turn_sensitivity;				//視点回転の感度
};

