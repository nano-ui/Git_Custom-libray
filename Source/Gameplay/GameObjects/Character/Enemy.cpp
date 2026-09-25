#include "Enemy.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Movement\MovementComponent.h"
#include "Gameplay\Components\Collision\BoneCapsuleColliderComponent.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"

#include <Windows.h>

static AutoRegister<Enemy> auto_register_enemy("Enemy");

//コンストラクタ
Enemy::Enemy()
	:head_collider_component(nullptr)
	,body_collider_component(nullptr)
{
	SetClassName("Enemy");
}

//デストラクタ
Enemy::~Enemy() = default;

//初期化処理
void Enemy::Initialize()
{
	SetupComponent();

	//基底クラスの初期化
	Character::Initialize();

	SetupColliders();
}

//更新処理
void Enemy::Update(float elapsed_time)
{
	Character::Update(elapsed_time);
	UpdateCollider();
}

//デバッグ描画
void Enemy::RenderDebug(ShapeRenderer* renderer)
{

}

//シリアライズ登録
void Enemy::SetupSerialization()
{

}

//インスペクター登録
void Enemy::SetupInspector()
{

}

// アニメーション終了イベント
void Enemy::OnAnimationEnd(uint32_t state_key)
{

}

//衝突コールバック処理
void Enemy::OnCollisionHit(const CollisionResult& result)
{

}

//コンポーネント群のセットアップ
void Enemy::SetupComponent()
{
	transform_component = GetComponent<TransformComponent>();
	if (!transform_component)transform_component = AddComponent<TransformComponent>();

	model_component = GetComponent<ModelComponent>();
	if (!model_component)model_component = AddComponent<ModelComponent>();

	if (model_component && transform_component)
	{
		model_component->SetTransformComponent(transform_component);

		//モデルがロードされていない場合
		if (model_component->GetModelPath().empty())
		{
			const std::string default_model_path = "Data/Model/Character/Enemy/Rampage_Elemental.gltf";
			model_component->LoadModel(default_model_path);
		}

	}
	else
	{
		OutputDebugStringA("[Enemy エラー] SetupComponents: 必要な基本コンポーネントの生成に失敗しました。\n");
	}
}

//部位別ボーン追従コライダーのセットアップ
void Enemy::SetupColliders()
{
	//ボーンの追従カプセルコライダーなどのセットアップ

}

//コライダー更新処理
void Enemy::UpdateCollider()
{
	if (!model_component)
	{
		OutputDebugStringA("[Enemy 警告] SetupColliders: model_component が nullptr のためコライダーを設定できません。\n");
		return;
	}

	//頭部用カプセルコライダーの生成とボーンの紐づけ
	head_collider_component = GetComponent<BoneCapsuleColliderComponent>();
	if (!head_collider_component)
	{
		head_collider_component = AddComponent<BoneCapsuleColliderComponent>();
	}

	if (head_collider_component)
	{
		head_collider_component->SetModelComponent(model_component);
		head_collider_component->SetAttachBone("rock_spikes_01_mid_top");
		head_collider_component->SetRadius(head_collider_radius);
		head_collider_component->SetHeight(head_collider_height);
		head_collider_component->SetAttribute(ColliderAttribute::Collision);
		head_collider_component->SetListener(this);
		AddCollider(head_collider_component->GetRawCollider());
	}
	else
	{
		OutputDebugStringA("[Enemy エラー] SetupColliders: head_collider_component の生成に失敗しました。\n");
	}

}
