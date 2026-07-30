#include "FollowCameraComponent.h"
#include "Camera.h"
#include "Engine\Core\Input.h"

#include <Windows.h>
#include <cmath>
#include <algorithm>
#include <imgui.h>

//コンストラクタ
FollowCameraComponent::FollowCameraComponent()
{
	Initialize();
}

//デストラクタ
FollowCameraComponent::~FollowCameraComponent()
{
}

//初期化処理
void FollowCameraComponent::Initialize()
{
	rotation_angle = DirectX::XMFLOAT2(DirectX::XMConvertToRadians(INITIAL_PITCH_DEGREE), 0.0f);
	distance = DEFAULT_DISTANCE;
	turn_sensitivity = DEFAULT_TURN_SENSITIVITY;
	look_at_offset = DirectX::XMFLOAT3(0.0f, 0.0f/*DEFAULT_OFFSET_Y*/, 0.0f);
	follow_speed = DEFAULT_FOLLOW_SPEED;
}

//更新処理
void FollowCameraComponent::Update(float elapsed_time)
{
	std::shared_ptr<Camera> camera = target_camera.lock();
	if (!camera)
	{
		OutputDebugStringA("[FollowCameraComponent] エラー: 制御対象のカメラ(target_camera)が設定されていないか破棄されています。\n");
		return;
	}

	// 追従対象の位置座標を取得
	DirectX::XMFLOAT3 target_pos = {};
	if (!target_position_getter || !target_position_getter(target_pos))
	{
		OutputDebugStringA("[FollowCameraComponent エラー] 追従対象の位置座標(target_position_getter)が未設定か、対象が破棄されています。\n");
		return;
	}

	//座標の非数(NaN)チェック
	if (std::isnan(target_pos.x) || std::isnan(target_pos.y) || std::isnan(target_pos.z)) 
	{
		OutputDebugStringA("[FollowCameraComponent エラー] 追従対象の座標に NaN が検出されました。\n");
		return;
	}

	//マウス操作による回転角度の更新
	UpdateMouseRotation(elapsed_time);

	//理想の注視点(Focus)と理想のカメラ視点(Eye)を算出
	DirectX::XMFLOAT3 ideal_focus = DirectX::XMFLOAT3(
		target_pos.x + look_at_offset.x,
		target_pos.y + look_at_offset.y,
		target_pos.z + look_at_offset.z
	);
	DirectX::XMFLOAT3 ideal_eye = CalculateOrbitEyePosition(ideal_focus);

	DirectX::XMFLOAT3 smoothed_eye;
	DirectX::XMFLOAT3 smoothed_focus;

	//初回・切替直後はワープ、2フレーム目以降は「視点」と「注視点」を揃えて滑らかに補間
	if (is_first_frame)
	{
		smoothed_eye = ideal_eye;
		smoothed_focus = ideal_focus;
		is_first_frame = false; // フラグを下ろす
		OutputDebugStringA("[FollowCameraComponent] カメラを理想位置へ即時ワープ配置しました。\n");
	}
	else
	{
		//視点の補間
		DirectX::XMFLOAT3 current_eye = camera->GetEye();
		smoothed_eye = InterpolatePosition(current_eye, ideal_eye, follow_speed, elapsed_time);

		//注視点も同じ速度で滑らかに補間してカメラ向きのガタつき（画面酔い）を防ぐ
		DirectX::XMFLOAT3 current_focus = camera->GetFocus();
		smoothed_focus = InterpolatePosition(current_focus, ideal_focus, follow_speed, elapsed_time);
	}

	//補間された視点と注視点を使ってカメラ行列を更新
	DirectX::XMFLOAT3 default_up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	camera->SetLookAt(smoothed_eye, smoothed_focus, default_up);
}

//ImGui描画
void FollowCameraComponent::RenderGui()
{
	if (ImGui::TreeNode(u8"追従カメラ (オービタル)"))
	{
		std::shared_ptr<Camera> camera = target_camera.lock();
		if (camera)
		{
			DirectX::XMFLOAT3 current_eye = camera->GetEye();
			ImGui::InputFloat3(u8"カメラ位置", &current_eye.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
		}
		ImGui::DragFloat(u8"カメラ距離", &distance, 0.1f, 1.0f, 50.0f);
		ImGui::DragFloat(u8"感度", &turn_sensitivity, 0.01f, 0.01f, 2.0f);
		ImGui::DragFloat3(u8"注視点オフセット", &look_at_offset.x, 0.1f);
		ImGui::SliderFloat(u8"追従速度", &follow_speed, 0.1f, 30.0f);
		ImGui::Text(u8"Pitch(上下): %.2f, Yaw(左右): %.2f", rotation_angle.x, rotation_angle.y);
		ImGui::TreePop();
	}
}

//追従対象の位置設定
void FollowCameraComponent::SetTarget(const std::function<bool(DirectX::XMFLOAT3&)>& getter)
{
	target_position_getter = getter;
}

//制御対象のカメラを設定
void FollowCameraComponent::SetCamera(const std::shared_ptr<Camera>& camera)
{
	target_camera = camera;
}

//マウス入力による回転角の更新
void FollowCameraComponent::UpdateMouseRotation(float elapsed_time)
{
	if (Input::Instance().IsKeyPress(VK_RBUTTON))
	{
		float delta_x = Input::Instance().GetMouseDeltaX();
		float delta_y = Input::Instance().GetMouseDeltaY();

		// 飛び値の制御
		constexpr float max_mouse_delta = 100.0f;
		delta_x = std::clamp(delta_x, -max_mouse_delta, max_mouse_delta);
		delta_y = std::clamp(delta_y, -max_mouse_delta, max_mouse_delta);

		rotation_angle.y += delta_x * turn_sensitivity * elapsed_time;
		rotation_angle.x += delta_y * turn_sensitivity * elapsed_time;

		constexpr float safe_pitch_limit_deg = 80.0f;
		const float min_pitch_rad = DirectX::XMConvertToRadians(-safe_pitch_limit_deg);
		const float max_pitch_rad = DirectX::XMConvertToRadians(safe_pitch_limit_deg);
		rotation_angle.x = std::clamp(rotation_angle.x, min_pitch_rad, max_pitch_rad);

		//左右回転角(Yaw)を -π ～ +π の範囲に正規化
		while (rotation_angle.y > DirectX::XM_PI)  rotation_angle.y -= DirectX::XM_2PI;
		while (rotation_angle.y < -DirectX::XM_PI) rotation_angle.y += DirectX::XM_2PI;
	}
}

//ターゲット座標と角度・距離から理想のカメラ位置を極座標計算
DirectX::XMFLOAT3 FollowCameraComponent::CalculateOrbitEyePosition(const DirectX::XMFLOAT3& focus_pos) const 
{
	//距離(distance)が0以下になってゼロ除算を起こさないようガード
	float safe_distance = (std::max)(distance, 1.0f);

	//オイラー角から回転クォータニオンを生成
	DirectX::XMVECTOR rotation_quaternion = DirectX::XMQuaternionRotationRollPitchYaw(
		rotation_angle.x,
		rotation_angle.y,
		0.0f
	);

	rotation_quaternion = DirectX::XMQuaternionNormalize(rotation_quaternion);

	//カメラの基準ローカルオフセットベクトルを設定
	DirectX::XMVECTOR local_offset = DirectX::XMVectorSet(0.0f, 0.0f, -safe_distance, 0.0f);

	//クォータニオンを用いてローカルオフセットベクトルを 3D 回転
	DirectX::XMVECTOR rotated_offset = DirectX::XMVector3Rotate(local_offset, rotation_quaternion);

	//注視点の座標ベクトルに回転後のオフセットを加算して視点座標を算出
	DirectX::XMVECTOR focus_vector = DirectX::XMLoadFloat3(&focus_pos);
	DirectX::XMVECTOR eye_vector = DirectX::XMVectorAdd(focus_vector, rotated_offset);
	DirectX::XMFLOAT3 eye_pos;
	DirectX::XMStoreFloat3(&eye_pos, eye_vector);

	//地中沈み込み防止チェック
	if (eye_pos.y < MIN_CAMERA_HEIGHT)
	{
		OutputDebugStringA("[FollowCameraComponent 警告] カメラ座標が地面以下を指したため高度を補正しました。\n");
		eye_pos.y = MIN_CAMERA_HEIGHT;
	}

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
