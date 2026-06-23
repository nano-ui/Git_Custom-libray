#pragma once
#include "Camera.h"

class StandCamera : public Camera
{
public:
	//コンストラクタ
	StandCamera();

	//デストラクタ
	~StandCamera();

	//初期化
	void Initialize()override;

	//更新
	void Update(float elapsed_time)override;

	//ImGui描画
	void RenderGui()override;

	//カメラ座標と注視点の設定
	void SetPositionAndFocus(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target);

private:
	bool is_rotation = true;		//自動旋回演出フラグ
	float rotation_speed = 0.0f;	//旋回速度
	float current_angle = 0.0f;		//旋回角度
	float camera_radius = 10.0f;	//旋回半径距離
	DirectX::XMFLOAT3 initial_eye;	//視点座標保持
};

