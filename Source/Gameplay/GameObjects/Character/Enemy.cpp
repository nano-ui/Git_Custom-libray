#include "Enemy.h"

//コンストラクタ
Enemy::Enemy()
	:collider_component(nullptr)
	,collider_radius(1.0f)
	,collider_offset(0.0f,1.0f,0.0f)
{
	SetClassName("エネミー");
}

//デストラクタ
Enemy::~Enemy() = default;

//初期化処理
void Enemy::Initialize()
{
	Character::Initialize();
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

//コライダー更新処理
void Enemy::UpdateCollider()
{

}
