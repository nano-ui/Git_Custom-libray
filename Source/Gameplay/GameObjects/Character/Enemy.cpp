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
}

//更新処理
void Enemy::Update(float elapsed_time)
{
	Character::Update(elapsed_time);
}

//デバッグ描画
void Enemy::RenderDebug(ShapeRenderer* renderer)
{
	Character::RenderDebug(renderer);
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