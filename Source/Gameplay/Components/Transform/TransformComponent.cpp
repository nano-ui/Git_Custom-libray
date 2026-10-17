#include "TransformComponent.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"

#include <imgui.h>
#include <Windows.h>
#include <cmath>

//コンストラクタ
TransformComponent::TransformComponent()
	:position({ 0.0f,0.0f,0.0f })
	, rotation({ 0.0f,0.0f,0.0f })
	, quaternion({ 0.0f,0.0f,0.0f,1.0f })
	, scale({ 1.0f,1.0f,1.0f })
	, world_matrix(DirectX::XMMatrixIdentity())
	, is_dirty(true)
{
	SetComponentName(u8"トランスフォーム");
}

//仮想デストラクタ
TransformComponent::~TransformComponent()
{
}

//初期化処理
void TransformComponent::Initialize()
{
	Component::Initialize();

	position = { 0.0f,0.0f,0.0f };
	rotation = { 0.0f,0.0f,0.0f };
	quaternion = { 0.0f,0.0f,0.0f,1.0f };
	scale = { 1.0f,1.0f,1.0f };
	world_matrix = DirectX::XMMatrixIdentity();
	is_dirty = true;

	UpdateWorldMatrix();
}

//更新処理
void TransformComponent::Update(float elapsed_time)
{
	if (!is_active)return;
	if (is_dirty)UpdateWorldMatrix();
}

//ImGuiデバッグ描画処理
void TransformComponent::RenderGui()
{
	if (!is_active)return;

	//if (ImGui::TreeNode(GetComponentName().c_str()))
	//{
	//	if (ImGui::DragFloat3(u8"位置", &position.x, 0.1f))is_dirty = true;

	//	DirectX::XMFLOAT3 euler_dug = {
	//		DirectX::XMConvertToDegrees(rotation.x),
	//		DirectX::XMConvertToDegrees(rotation.y),
	//		DirectX::XMConvertToDegrees(rotation.z)
	//	};

	//	if (ImGui::DragFloat3(u8"回転", &euler_dug.x, 1.0f))
	//	{
	//		rotation.x = DirectX::XMConvertToRadians(euler_dug.x);
	//		rotation.y = DirectX::XMConvertToRadians(euler_dug.y);
	//		rotation.z = DirectX::XMConvertToRadians(euler_dug.z);
	//		is_dirty = true;
	//	}

	//	if (ImGui::DragFloat3(u8"拡大率", &scale.x, 0.1f))is_dirty = true;
	//	ImGui::TreePop();
	//}
}

//JSON保存用変数登録
void TransformComponent::SetupSerialization(JsonSerializer* serializer)
{
	serializer->RegisterVariable<DirectX::XMFLOAT3>(u8"座標", &position, SerializeTarget::Base);
	serializer->RegisterVariable<DirectX::XMFLOAT3>(u8"回転", &rotation, SerializeTarget::Base);
	serializer->RegisterVariable<DirectX::XMFLOAT3>(u8"拡大", &scale, SerializeTarget::Base);
}

//GuiInspector専用のインスペクター項目登録
void TransformComponent::SetupInspector(GuiInspector* inspector)
{
	inspector->RegisterVariable(u8"座標", &position, GetComponentName());
	inspector->RegisterVariable(u8"回転", &rotation, GetComponentName());
	inspector->RegisterVariable(u8"拡大", &scale, GetComponentName());
}

//座標の設定
void TransformComponent::SetPosition(const DirectX::XMFLOAT3& pos)
{
	if (!is_active)return;
	position = pos;
	is_dirty = true;
}

//オイラー角の設定
void TransformComponent::SetRotation(const DirectX::XMFLOAT3& rot)
{
	if (std::isnan(rot.x) || std::isnan(rot.y) || std::isnan(rot.z))
	{
		OutputDebugStringA("[TransformComponent エラー] SetRotation: オイラー角に NaN が検出されたため処理をスキップしました。\n");
		return;
	}

	rotation = rot;

	DirectX::XMVECTOR quaternion_vec = DirectX::XMQuaternionRotationRollPitchYaw(
		rotation.x,
		rotation.y,
		rotation.z
	);
	DirectX::XMStoreFloat4(&quaternion, quaternion_vec);

	is_dirty = true;
}

//クォータニオンの設定
void TransformComponent::SetRotationQuaternion(const DirectX::XMFLOAT4& rot)
{
	if (std::isnan(rot.x) || std::isnan(rot.y) || std::isnan(rot.z) || std::isnan(rot.w))
	{
		OutputDebugStringA("[TransformComponent エラー] SetRotationQuaternion: クォータニオンに NaN が検出されたため処理をスキップしました。\n");
		return;
	}

	quaternion = rot;
	UpdateEulerFromQuaternion();
	is_dirty = true;
}

//スケールの設定
void TransformComponent::SetScale(const DirectX::XMFLOAT3& scl)
{
	scale = scl;
	is_dirty = true;
}

//行列とクォータニオンの計算処理
void TransformComponent::UpdateWorldMatrix()
{
	//オイラー角からクォータニオンへの変換
	DirectX::XMVECTOR quaternion_vec = DirectX::XMLoadFloat4(&quaternion);

	//各変換行列の作成
	DirectX::XMMATRIX matrix_scale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX matrix_rotation = DirectX::XMMatrixRotationQuaternion(quaternion_vec);
	DirectX::XMMATRIX matrix_translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//ワールド行列の合成
	world_matrix = matrix_scale * matrix_rotation * matrix_translation;

	//計算完了したためフラグ解除
	is_dirty = false;
}

//クォータニオンからオイラー角を算出して同期
void TransformComponent::UpdateEulerFromQuaternion()
{
	DirectX::XMVECTOR quat_vec = DirectX::XMLoadFloat4(&quaternion);
	DirectX::XMMATRIX rot_matrix = DirectX::XMMatrixRotationQuaternion(quat_vec);
	DirectX::XMFLOAT4X4 m = {};
	DirectX::XMStoreFloat4x4(&m, rot_matrix);

	constexpr float gimbal_threshold = 0.99999f;
	constexpr float half_pi = 1.57079632679f;

	float sin_pitch = -m._32;

	if (sin_pitch > gimbal_threshold)
	{
		rotation.x = half_pi;
		rotation.y = std::atan2(-m._21, m._11);
		rotation.z = 0.0f;
	}
	else if (sin_pitch < -gimbal_threshold)
	{
		rotation.x = -half_pi;
		rotation.y = std::atan2(-m._21, m._11);
		rotation.z = 0.0f;
	}
	else
	{
		rotation.x = std::asin(sin_pitch);
		rotation.y = std::atan2(m._31, m._33);
		rotation.z = std::atan2(m._12, m._22);
	}
}
