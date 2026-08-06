#include "TransformComponent.h"
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

	if (ImGui::TreeNode(GetComponentName().c_str()))
	{
		if (ImGui::DragFloat3(u8"位置", &position.x, 0.1f))is_dirty = true;

		DirectX::XMFLOAT3 euler_dug = {
			DirectX::XMConvertToDegrees(rotation.x),
			DirectX::XMConvertToDegrees(rotation.y),
			DirectX::XMConvertToDegrees(rotation.z)
		};

		if (ImGui::DragFloat3(u8"回転", &euler_dug.x, 1.0f))
		{
			rotation.x = DirectX::XMConvertToRadians(euler_dug.x);
			rotation.y = DirectX::XMConvertToRadians(euler_dug.y);
			rotation.z = DirectX::XMConvertToRadians(euler_dug.z);
			is_dirty = true;
		}

		if (ImGui::DragFloat3(u8"拡大率", &scale.x, 0.1f))is_dirty = true;
		ImGui::TreePop();
	}
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
		OutputDebugStringA("[TransformComponent エラー] SetRotation: オイラー角に NaN が検出されたため処理を復元・スキップしました。\n");
		return;
	}

	rotation = rot;
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
	DirectX::XMVECTOR quaternion_vec = DirectX::XMQuaternionRotationRollPitchYaw(
		rotation.x,
		rotation.y,
		rotation.z
	);
	DirectX::XMStoreFloat4(&quaternion, quaternion_vec);

	//各変換行列の作成
	DirectX::XMMATRIX matrix_scale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX matrix_rotation = DirectX::XMMatrixRotationQuaternion(quaternion_vec);
	DirectX::XMMATRIX matrix_translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//ワールド行列の合成
	world_matrix = matrix_scale * matrix_rotation * matrix_translation;

	//計算完了したためフラグ解除
	is_dirty = false;
}
