#include "BoneCapsuleColliderComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"

#include <windows.h>

//コンストラクタ
BoneCapsuleColliderComponent::BoneCapsuleColliderComponent()
{
	SetComponentName(u8"ボーン追従カプセルコライダー");
	capsule_collider = std::make_unique<CapsuleCollider>();
}

//デストラクタ
BoneCapsuleColliderComponent::~BoneCapsuleColliderComponent() = default;

//初期化処理
void BoneCapsuleColliderComponent::Initialize()
{
	Component::Initialize();

	if (target_model.expired())
	{
		OutputDebugStringA("[BoneCapsuleColliderComponent 警告] Initialize: target_model が未設定です。SetModelComponent で設定してください。\n");
	}

	UpdateTransform();
}

//更新処理
void BoneCapsuleColliderComponent::Update(float elapsed_time)
{
	if (!is_active)return;
	UpdateTransform();
}

//デバッグ描画
void BoneCapsuleColliderComponent::RenderDebug(ShapeRenderer* renderer)
{
	if (!renderer || !capsule_collider || !capsule_collider->is_active)return;

	DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&capsule_collider->start_center);
	DirectX::XMVECTOR v_end = DirectX::XMLoadFloat3(&capsule_collider->end_center);

	DirectX::XMVECTOR v_center = DirectX::XMVectorScale(DirectX::XMVectorAdd(v_start, v_end), 0.5f);
	DirectX::XMFLOAT3 center_pos = {};
	DirectX::XMStoreFloat3(&center_pos, v_center);

	DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(v_end, v_start);
	float actual_height = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
	float total_height = actual_height + (radius * 2.0f);

	//カプセルの向き
	DirectX::XMVECTOR default_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR dir = (actual_height > 1e-4f) ? DirectX::XMVector3Normalize(diff) : default_up;

	DirectX::XMVECTOR rot_quat = DirectX::XMQuaternionIdentity();
	DirectX::XMVECTOR rot_axis = DirectX::XMVector3Cross(default_up, dir);
	float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(default_up, dir));

	if (dot < -0.9999f)
	{
		rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XM_PI);
	}
	else if (dot < 0.9999f)
	{
		float angle = acosf(dot);
		rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVector3Normalize(rot_axis), angle);
	}

	DirectX::XMFLOAT4 rotation = {};
	DirectX::XMStoreFloat4(&rotation, rot_quat);

	constexpr DirectX::XMFLOAT4 debug_color = { 1.0f,0.2f,0.2f,1.0f };
	renderer->DrawCapsule(
		center_pos,
		rotation,
		radius,
		total_height,
		debug_color,
		ShapeDrawMode::Wireframe
	);
}

//GUIインスペクター登録
void BoneCapsuleColliderComponent::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
	if (inspector)
	{
		inspector->RegisterVariable(u8"半径", &radius, u8"ボーンコライダー設定");
		inspector->RegisterVariable(u8"長さ", &height, u8"ボーンコライダー設定");
		inspector->RegisterVariable(u8"オフセット", &offset, u8"ボーンコライダー設定");
	}
}

//シリアライズ登録
void BoneCapsuleColliderComponent::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	if (serializer)
	{
		serializer->RegisterVariable(u8"ボーン名_始点", &start_bone_name);
		serializer->RegisterVariable(u8"ボーン名_終点", &end_bone_name);
		serializer->RegisterVariable(u8"半径", &radius);
		serializer->RegisterVariable(u8"長さ", &height);
		serializer->RegisterVariable(u8"オフセット", &offset);
	}
}

//追従対象のモデルコンポーネント登録
void BoneCapsuleColliderComponent::SetModelComponent(const std::shared_ptr<ModelComponent>& model_comp)
{
	target_model = model_comp;
}

//単一ボーン設定
void BoneCapsuleColliderComponent::SetAttachBone(const std::string& bone_name)
{
	start_bone_name = bone_name;
	end_bone_name.clear();
}

//2ボーン連携設定
void BoneCapsuleColliderComponent::SetAttachBones(const std::string& start_name, const std::string end_name)
{
	start_bone_name = start_name;
	end_bone_name = end_name;
}

//属性設定
void BoneCapsuleColliderComponent::SetAttribute(ColliderAttribute attr)
{
	if (capsule_collider)
	{
		capsule_collider->attribute = attr;
	}
}

//リスナー設定
void BoneCapsuleColliderComponent::SetListener(ICollisionListener* listener)
{
	if (capsule_collider)
	{
		capsule_collider->listener = listener;
	}
}

//ボーン追従座標更新処理
void BoneCapsuleColliderComponent::UpdateTransform()
{
	if (!capsule_collider)return;

	std::shared_ptr<ModelComponent> model_comp = target_model.lock();
	if (!model_comp)return;

	//前回の座標を保存
	capsule_collider->old_start_center = capsule_collider->start_center;
	capsule_collider->old_end_center = capsule_collider->end_center;
	capsule_collider->radius = radius;

	//2ボーン連結時
	if (!end_bone_name.empty())
	{
		DirectX::XMFLOAT3 start_pos = {};
		DirectX::XMFLOAT3 end_pos = {};

		bool has_start = model_comp->GetBoneWorldPosition(start_bone_name, start_pos);
		bool has_end = model_comp->GetBoneWorldPosition(end_bone_name, end_pos);

		if (has_start && has_end)
		{
			capsule_collider->start_center = start_pos;
			capsule_collider->end_center = end_pos;
		}
		else
		{
			printf_s("[BoneCapsuleColliderComponent 警告] UpdateTransform: 連結ボーン座標の取得に失敗しました。\n");
		}
	}
	//単一ボーン時
	else if (!start_bone_name.empty())
	{
		DirectX::XMFLOAT4X4 bone_world = {};
		if (model_comp->GetBoneWorldTransform(start_bone_name, bone_world))
		{
			DirectX::XMMATRIX mat_bone = DirectX::XMLoadFloat4x4(&bone_world);

			//ローカル始点と終点
			DirectX::XMVECTOR local_start = DirectX::XMVectorSet(offset.x, offset.y, offset.z, 1.0f);
			DirectX::XMVECTOR local_end = DirectX::XMVectorSet(offset.x, offset.y + height, offset.z, 1.0f);

			//ボーンのワールド行列で座標変換
			DirectX::XMVECTOR world_start = DirectX::XMVector3TransformCoord(local_start, mat_bone);
			DirectX::XMVECTOR world_end = DirectX::XMVector3TransformCoord(local_end, mat_bone);

			DirectX::XMStoreFloat3(&capsule_collider->start_center, world_start);
			DirectX::XMStoreFloat3(&capsule_collider->end_center, world_end);
		}
		else
		{
			printf_s("[BoneCapsuleColliderComponent 警告] UpdateTransform: 単一ボーン行列の取得に失敗しました。\n");
		}
	}
}
