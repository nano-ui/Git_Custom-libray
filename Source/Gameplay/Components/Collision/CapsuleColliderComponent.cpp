#include "CapsuleColliderComponent.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

//コンストラクタ
CapsuleColliderComponent::CapsuleColliderComponent()
	:radius(0.5f)
	,height(1.0f)
{
	SetComponentName(u8"カプセルコライダーコンポーネント");
	capsule_collider.type = ColliderType::Capsule;
	capsule_collider.radius = radius;
}

//シリアライズ登録
void CapsuleColliderComponent::SetupSerialization(JsonSerializer* serializer)
{
	ColliderComponent::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"コライダー半径", &radius);
	serializer->RegisterVariable(u8"コライダー高さ", &height);
}

//インスペクター登録
void CapsuleColliderComponent::SetupInspector(GuiInspector* inspector)
{
	ColliderComponent::SetupInspector(inspector);
	inspector->RegisterVariable(u8"コライダー半径", &radius, GetComponentName());
	inspector->RegisterVariable(u8"コライダー高さ", &height, GetComponentName());
	inspector->RegisterText(u8"カプセルの始点", &capsule_collider.start_center, GetComponentName());
	inspector->RegisterText(u8"カプセルの終点", &capsule_collider.end_center, GetComponentName());
	inspector->RegisterText(u8"前回の始点", &capsule_collider.old_start_center, GetComponentName());
	inspector->RegisterText(u8"前回の終点", &capsule_collider.old_end_center, GetComponentName());
}

//半径の設定
void CapsuleColliderComponent::SetRadius(float r)
{
	radius = r;
	capsule_collider.radius = r;
}

//座標の同期処理
void CapsuleColliderComponent::UpdateColliderTransform(const DirectX::XMFLOAT3& world_pos, const DirectX::XMFLOAT4& world_rot)
{
	//前フレーム座標を記録
	capsule_collider.old_start_center = capsule_collider.start_center;
	capsule_collider.old_end_center = capsule_collider.end_center;

	//下端中心の算出
	capsule_collider.start_center.x = world_pos.x + offset.x;
	capsule_collider.start_center.y = world_pos.y + offset.y;
	capsule_collider.start_center.z = world_pos.z + offset.z;

	//上端中心の算出
	capsule_collider.end_center.x = capsule_collider.start_center.x;
	capsule_collider.end_center.y = capsule_collider.start_center.y + height;
	capsule_collider.end_center.z = capsule_collider.start_center.z;

	//半径の適用
	capsule_collider.radius = radius;
}
