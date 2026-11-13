#include "ObjectGizmo.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Core\Input.h"
#include "Gameplay\Components\Transform\TransformComponent.h"

//3Dオブジェクトのギズモ表示
void ObjectGizmo::Draw3dGizumo(
	TransformComponent* transform,
	Camera* camera, 
	DirectX::XMFLOAT2 window_size,
	DirectX::XMFLOAT2 window_pos)
{
	//ガード処理
	if (!transform || !camera)
	{
		LOG_ERROR({ u8"設定" }, u8"設定されたポインタがnullです");
		return;
	}

	//キーボード入力による操作モードの切り替え
	if (Input::Instance().IsKeyTrigger('W')) operation = ImGuizmo::TRANSLATE;
	if (Input::Instance().IsKeyTrigger('E')) operation = ImGuizmo::ROTATE;
	if (Input::Instance().IsKeyTrigger('R')) operation = ImGuizmo::SCALE;

	//描画設定と行列合成
	DirectX::XMVECTOR v_pos = DirectX::XMLoadFloat3(&transform->GetPosition());
	DirectX::XMVECTOR v_qua = DirectX::XMLoadFloat4(&transform->GetQuaternion());
	DirectX::XMFLOAT3 scl = transform->GetScale();
	DirectX::XMVECTOR v_scl = DirectX::XMLoadFloat3(&scl);
	DirectX::XMMATRIX transform_matrix = DirectX::XMMatrixAffineTransformation(v_scl, DirectX::XMVectorZero(), v_qua, v_pos);
	DirectX::XMFLOAT4X4 world_matrix;
	DirectX::XMStoreFloat4x4(&world_matrix, transform_matrix);

	//操作と分解
	const DirectX::XMFLOAT4X4& view = camera->GetView();
	const DirectX::XMFLOAT4X4& projection = camera->GetProjection();
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetRect(window_pos.x, window_pos.y, window_size.x, window_size.y);
	ImGuizmo::Manipulate(
		&view._11,
		&projection._11,
		operation,
		ImGuizmo::LOCAL,
		&world_matrix._11
	);
	if (ImGuizmo::IsUsing())
	{
		transform_matrix = DirectX::XMLoadFloat4x4(&world_matrix);
		DirectX::XMVECTOR matrix_scale;
		DirectX::XMVECTOR matrix_quaternion;
		DirectX::XMVECTOR matrix_translation;
		DirectX::XMMatrixDecompose(&matrix_scale, &matrix_quaternion, &matrix_translation, transform_matrix);
		DirectX::XMFLOAT3 translation = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT4 quaternion = { 0.0f,0.0f,0.0f,1.0f };
		DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f };
		DirectX::XMStoreFloat3(&translation, matrix_translation);
		DirectX::XMStoreFloat4(&quaternion, matrix_quaternion);
		DirectX::XMStoreFloat3(&scale, matrix_scale);
		transform->SetPosition(translation);
		transform->SetRotationQuaternion(quaternion);
		transform->SetScale(scale);
	}
}
