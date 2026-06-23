#include "StandCamera.h"

#include <imgui.h>
#include <cmath>

//コンストラクタ
StandCamera::StandCamera()
{

}

//デストラクタ
StandCamera::~StandCamera()
{
}

//初期化
void StandCamera::Initialize()
{
	//カメラのデフォルト位置と注視点定数の定義
	static constexpr DirectX::XMFLOAT3 default_eye = { 0.0f,3.0f,-12.0f };
	static constexpr DirectX::XMFLOAT3 default_focus = { 0.0f,1.0f,0.0f };
	initial_eye = default_eye;
	eye = initial_eye;
	focus = default_focus;
	up = { 0.0f,1.0f,0.0f };

	//旋回運動用の半径と初期角度の算出
	float dx = eye.x - focus.x;
	float dz = eye.z - focus.z;
	camera_radius = sqrtf(dx * dx + dz * dz);
	current_angle = atan2f(dz, dx);

	//画角とパースペクティブ行列の構築
	static constexpr float default_fov = 45.0f;
	static constexpr float default_near = 0.1f;
	static constexpr float default_far = 1000.0f;
	static constexpr float default_aspect = 1280.0f / 720.0f;
	SetPerspectiveFov(DirectX::XMConvertToRadians(default_fov), default_aspect, default_near, default_far);
	SetLookAt(eye, focus, up);
}

//更新
void StandCamera::Update(float elapsed_time)
{
	//自動旋回フラグ有効時の座標計算
	if (is_rotation)
	{
		current_angle += rotation_speed * elapsed_time;
		eye.x = focus.x + camera_radius * cosf(current_angle);
		eye.z = focus.z + camera_radius * sinf(current_angle);
		SetLookAt(eye, focus, up);
	}
}

//ImGui描画
void StandCamera::RenderGui()
{
#ifdef USE_IMGUI
	//デバッグウィンドウの描画と変数編集
	ImGui::Begin("Stand Camera Debug");
	ImGui::Checkbox("Auto Rotation", &is_rotation);
	ImGui::DragFloat("Rotation Speed", &rotation_speed, 0.1f);
	ImGui::DragFloat3("Camera Eye", &eye.x, 0.1f);
	ImGui::DragFloat3("Camera Focus", &focus.x, 0.1f);
	if (!is_rotation)
	{
		SetLookAt(eye, focus, up);
		float dx = eye.x - focus.x;
		float dz = eye.z - focus.z;
		camera_radius = sqrtf(dx * dx + dz * dz);
		current_angle = atan2f(dz, dz);
	}
	ImGui::End();
#endif
}

//カメラ座標と注視点の設定
void StandCamera::SetPositionAndFocus(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target)
{
	//座標設定
	eye = position;
	focus = target;
	initial_eye = position;

	float dx = eye.x - focus.x;
	float dz = eye.z - focus.z;
	camera_radius = sqrtf(dx * dx + dz * dz);
	current_angle = atan2f(dz, dz);
	SetLookAt(eye, focus, up);
}
