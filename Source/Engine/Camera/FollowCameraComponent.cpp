#include "FollowCameraComponent.h"
#include "Camera.h"
#include "Gameplay\GameObjects\GameObject.h"

#include <Windows.h>
#include <cmath>
#include <imgui.h>

//コンストラクタ
FollowCameraComponent::FollowCameraComponent()
	:offset_position(DEFAULT_OFFSET_X, DEFAULT_OFFSET_Y, DEFAULT_OFFSET_Z)
	, look_at_offset(0.0f, 0.0f, 0.0f)
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
	offset_position = DirectX::XMFLOAT3(DEFAULT_OFFSET_X, DEFAULT_OFFSET_Y, DEFAULT_OFFSET_Z);
	look_at_offset = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
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

	DirectX::XMFLOAT3 target_pos = target_obj->GetPosition();

	//カメラ視点位置と注視点位置を計算
	DirectX::XMFLOAT3 ideal_eye = CalculateTargetEyePosition(*target_pos);
	DirectX::XMFLOAT3 ideal_focus = DirectX::XMFLOAT3(
		target_pos.x + look_at_offset.x,
		target_pos.y + look_at_offset.y,
		target_pos.z + look_at_offset.z
	);

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
	if (ImGui::TreeNode(u8"追従カメラ"))
	{
		ImGui::DragFloat3(u8"オフセット座標", &offset_position.x, 0.1f);
		ImGui::DragFloat3(u8"視点オフセット座標", &look_at_offset.x, 0.1f);
		ImGui::SliderFloat(u8"カメラ速度", &follow_speed, 0.1f, 20.0f);
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

//ターゲットの位置とオフセットから理想のカメラ位置を計算
DirectX::XMFLOAT3 FollowCameraComponent::CalculateTargetEyePosition(const DirectX::XMFLOAT3& target_pos) const
{
	return DirectX::XMFLOAT3(
		target_pos.x + offset_position.x,
		target_pos.y + offset_position.y,
		target_pos.z + offset_position.z
	);
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
