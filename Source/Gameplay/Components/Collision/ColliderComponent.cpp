#include "ColliderComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Engine\Collision\CollisionManager.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

#include <Windows.h>

//コンストラクタ
ColliderComponent::ColliderComponent()
	:offset({0.0f,0.0f,0.0f})
	,weight(1.0f)
	,attribute(ColliderAttribute::Collision)
	,is_registered(false)
{
	SetComponentName(u8"コライダーコンポーネント");
}

//デストラクタ
ColliderComponent::~ColliderComponent()
{
	UnregisterFromManager();
}

//初期化処理
void ColliderComponent::Initialize()
{
	Component::Initialize();
	if (target_transform.expired())
	{
		OutputDebugStringA("[ColliderComponent 警告] target_transform が未設定です。\n");
	}

	auto transform = target_transform.lock();
	if (transform)UpdateColliderTransform(transform->GetPosition(), transform->GetQuaternion());
	RegisterToManager();
}

//更新処理
void ColliderComponent::Update(float elapsed_time)
{
	if (!is_active)return;
	auto transform = target_transform.lock();
	if (transform)UpdateColliderTransform(transform->GetPosition(), transform->GetQuaternion());
}

//シリアライズ登録
void ColliderComponent::SetupSerialization(JsonSerializer* serializer)
{
	if (!serializer)return;
	serializer->RegisterVariable(u8"コライダーオフセット", &offset);
	serializer->RegisterVariable(u8"重力", &weight);
}

//inspector登録
void ColliderComponent::SetupInspector(GuiInspector* inspector)
{
	if (!inspector)return;
	inspector->RegisterVariable(u8"コライダーオフセット", &offset, GetComponentName());
	inspector->RegisterVariable(u8"重力", &weight, GetComponentName());
	inspector->RegisterText(u8"登録フラグ", &is_registered, GetComponentName());
}

//トランスフォームの設定
void ColliderComponent::SetTransformComponent(const std::shared_ptr<TransformComponent>& transform)
{
	target_transform = transform;
}

//衝突イベントの設定
void ColliderComponent::SetListener(ICollisionListener* listener)
{
	Collider* raw_collider = GetRawCollider();
	if (raw_collider)raw_collider->listener = listener;
}

//属性の設定
void ColliderComponent::SetAttribute(ColliderAttribute attr)
{
	attribute = attr;
	Collider* raw_collider = GetRawCollider();
	if (raw_collider)raw_collider->attribute = attr;
}

//重量の設定
void ColliderComponent::SetWeight(float w)
{
	weight = w;
	Collider* raw_collider = GetRawCollider();
	if (raw_collider)raw_collider->weight = w;
}

//偏り座標の設定
void ColliderComponent::SetOffset(const DirectX::XMFLOAT3& off)
{
	offset = off;
}

//CollisionManagerへの登録
void ColliderComponent::RegisterToManager()
{
	if (is_registered)return;
	Collider* raw_collider = GetRawCollider();
	CollisionManager* col_manager = ObjectManager::Instance().GetCollisionManager();
	if (raw_collider && col_manager)
	{
		col_manager->Register(raw_collider);
		is_registered = true;
	}
	else OutputDebugStringA("[ColliderComponent 警告] CollisionManager または Collider が無効のため登録できませんでした。\n");
}

//CollisionManagerへの解除
void ColliderComponent::UnregisterFromManager()
{
	if (!is_registered)return;
	Collider* raw_collider = GetRawCollider();
	CollisionManager* col_manager = ObjectManager::Instance().GetCollisionManager();
	if (raw_collider && col_manager)
	{
		col_manager->Remove(raw_collider);
		is_registered = false;
	}
}
