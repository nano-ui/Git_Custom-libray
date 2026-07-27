#include "FollowCameraComponent.h"
#include "Camera.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Engine\Core\Input.h"

#include <Windows.h>
#include <cmath>
#include <imgui.h>

//コンストラクタ
FollowCameraComponent::FollowCameraComponent()
	: rotation_angle(0.0f, 0.0f)
	, distance(DEFAULT_DISTANCE)
	, turn_sensitivity(DEFAULT_TURN_SENSITIVITY)
	, look_at_offset(0.0f, DEFAULT_OFFSET_Y, 0.0f)
	, follow_speed(DEFAULT_FOLLOW_SPEED)
{
}

//デストラクタ
FollowCameraComponent::~FollowCameraComponent()
{
}

//初期化処理
void FollowCameraComponent::Initialize()
{
	rotation_angle = DirectX::XMFLOAT2(0.0f, 0.0f);
	distance = DEFAULT_DISTANCE;
	turn_sensitivity = DEFAULT_TURN_SENSITIVITY;
	look_at_offset = DirectX::XMFLOAT3(0.0f, DEFAULT_OFFSET_Y, 0.0f);
	follow_speed = DEFAULT_FOLLOW_SPEED;
}

//更新処理
void FollowCameraComponent::Update(float elapsed_time)
{
	std::shared_ptr<Camera> camera = target_camera.lock();
	if (!camera)
	{
		OutputDebugStringA("[FollowCameraComponent] エラー: 制御対象のカメラ(target_camera)が設定されていないか、既に破棄されています。\n");
		return;
	}

	std::shared_ptr<const GameObject> target_obj = target_object.lock();
	if (!target_obj)
	{
		OutputDebugStringA("[FollowCameraComponent] 警告: 追従対象(target_object)が設定されていないか、既に破棄されています。\n");
		return;
	}

	//マウス操作による回転角度の更新
	UpdateMouseRotation(elapsed_time);

	//追従対象の位置を取得し、注視点を算出
	DirectX::XMFLOAT3 target_pos = target_obj->GetPosition();
	DirectX::XMFLOAT3 ideal_focus = DirectX::XMFLOAT3(
		target_pos.x + look_at_offset.x,
		target_pos.y + look_at_offset.y,
		target_pos.z + look_at_offset.z
	);

	//マウス回転角と距離から、理想のカメラ視点座標（Eye）を球面計算
	DirectX::XMFLOAT3 ideal_eye = CalculateOrbitEyePosition(ideal_focus);

	//現在のカメラ位置を取得し、補間移動
	DirectX::XMFLOAT3 current_eye = camera->GetEye();
	DirectX::XMFLOAT3 smothed_eye = InterpolatePosition(current_eye, ideal_eye, follow_speed, elapsed_time);

	//カメラ行列を更新
	DirectX::XMFLOAT3 default_up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	camera->SetLookAt(smothed_eye, ideal_eye, default_up);
}

//ImGui描画
void FollowCameraComponent::RenderGui()
{
	if (ImGui::TreeNode(u8"追従カメラ (オービタル)"))
	{
		ImGui::DragFloat(u8"カメラ距離", &distance, 0.1f, 1.0f, 50.0f);
		ImGui::DragFloat(u8"感度", &turn_sensitivity, 0.01f, 0.01f, 2.0f);
		ImGui::DragFloat3(u8"注視点オフセット", &look_at_offset.x, 0.1f);
		ImGui::SliderFloat(u8"追従速度", &follow_speed, 0.1f, 30.0f);
		ImGui::Text(u8"Pitch(上下): %.2f, Yaw(左右): %.2f", rotation_angle.x, rotation_angle.y);
		ImGui::TreePop();
	}
}

//対象のGameObject参照の設定
void FollowCameraComponent::SetTarget(const std::shared_ptr<const GameObject>& target_obj)
{
	target_object = target_obj;
}

//制御対象のカメラを設定
void FollowCameraComponent::SetCamera(const std::shared_ptr<Camera>& camera)
{
	target_camera = camera;
}

//マウス入力による回転角の更新
void FollowCameraComponent::UpdateMouseRotation(float elapsed_time)
{
	//マウスの右ボタンが押されている間、視点回転を計算
	if (Input::Instance().IsKeyPress(VK_RBUTTON))
	{
		float delta_x = Input::Instance().GetMouseDeltaX();
		float delta_y = Input::Instance().GetMouseDeltaY();

		rotation_angle.y += delta_x * turn_sensitivity * elapsed_time;
		rotation_angle.x += delta_y * turn_sensitivity * elapsed_time;

		constexpr float max_pitch_rad = DirectX::XMConvertToRadians(MAX_PITCH_DEGREE);
		if (rotation_angle.x > max_pitch_rad) rotation_angle.x = max_pitch_rad;
		if (rotation_angle.x < -max_pitch_rad) rotation_angle.x = -max_pitch_rad;
	}
}

//ターゲット座標と角度・距離から理想のカメラ位置を極座標計算
DirectX::XMFLOAT3 FollowCameraComponent::CalculateOrbitEyePosition(const DirectX::XMFLOAT3& focus_pos) const 
{
	float cos_pitch = std::cosf(rotation_angle.x);
	float sin_pitch = std::sinf(rotation_angle.x);
	float cos_yaw = std::cosf(rotation_angle.y);
	float sin_yaw = std::sinf(rotation_angle.y);

	//球面座標系の計算公式を用いて、ターゲット周囲の位置ベクトルを算出
	DirectX::XMFLOAT3 eye_pos;
	eye_pos.x = focus_pos.x + distance * cos_pitch * sin_yaw;
	eye_pos.y = focus_pos.y + distance * sin_pitch;
	eye_pos.z = focus_pos.z - distance * cos_pitch * cos_yaw;

	return eye_pos;
}

//フレームレート非依存の補間計算
DirectX::XMFLOAT3 FollowCameraComponent::InterpolatePosition(
	const DirectX::XMFLOAT3& current_pos,
	const DirectX::XMFLOAT3& target_pos,
	float speed,
	float elapsed_time) const
{
	float factor = 1.0f - std::exp(-speed * elapsed_time);

	DirectX::XMFLOAT3 result;
	result.x = current_pos.x + (target_pos.x - current_pos.x) * factor;
	result.y = current_pos.y + (target_pos.y - current_pos.y) * factor;
	result.z = current_pos.z + (target_pos.z - current_pos.z) * factor;

	return result;
}
