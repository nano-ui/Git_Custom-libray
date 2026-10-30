#include "Allie.h"
#include "Gameplay\GameObjects\ObjectFactory.h"
#include "Gameplay\Components\Tactics\SquadManager.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Engine\Core\Log\Log.h"

static AutoRegister<Allie> auto_register_class("Allie");
//コンストラクタ
Allie::Allie()
{
	SetClassName("Allie");
}

//デストラクタ
Allie::~Allie()
{
	SquadManager::Instance().UnregisterMember(this);
}

//初期化処理
void Allie::Initialize()
{
	Character::Initialize();
	SetupComponent();
	SquadManager::Instance().RegisterMember(this);
	target_position = { 0.0f,0.0f,0.0f };
	has_target = false;
}

//更新処理
void Allie::Update(float elapsed_time)
{
	Character::Update(elapsed_time);
	MoveTarget(elapsed_time);
}

//衝突処理
void Allie::OnCollisionHit(const CollisionResult& result)
{
	Character::OnCollisionHit(result);
}

//描画処理
void Allie::Render(ID3D11DeviceContext* context)
{
	Character::Render(context);
}

//デバッグ描画
void Allie::RenderDebug(ShapeRenderer* renderer)
{
	Character::RenderDebug(renderer);
}

// シリアライズ登録
void Allie::SetupSerialization()
{
	Character::SetupSerialization();
}

//インスペクター登録
void Allie::SetupInspector()
{
	Character::SetupInspector();
}

//コンポーネントの設定
void Allie::SetupComponent()
{
	Character::SetupComponent();
}

//ブラックボードに登録・初期化
void Allie::SetupBlackboard()
{
	Character::SetupBlackboard();
}

//アニメーション終了イベント
void Allie::OnAnimationEnd(uint32_t state_key)
{

}

//目標地点に向けて進む
void Allie::MoveTarget(float elapsed_time)
{
	if (!has_target)return;
	auto transform = GetComponent<TransformComponent>();
	if (!transform)return;
	DirectX::XMFLOAT3 my_pos = transform->GetPosition();
	DirectX::XMVECTOR v_my_pos = DirectX::XMLoadFloat3(&my_pos);
	DirectX::XMVECTOR v_target_pos = DirectX::XMLoadFloat3(&target_position);
	DirectX::XMVECTOR v = DirectX::XMVectorSubtract(v_target_pos, v_my_pos);
	v = DirectX::XMVector3Normalize(v);
	float vector_x = DirectX::XMVectorGetX(v);
	float vector_z = DirectX::XMVectorGetZ(v);
	Character::Move(elapsed_time, vector_x, vector_z);
	Character::Tuen(elapsed_time, vector_x, vector_z);
}

//デフォルトパスの取得
std::string Allie::GetDefaultModelPath() const
{
	LOG_NORMAL({ u8"設定" }, u8"デフォルトパスが設定されました");
	return "Data/Model/Character/Player/Greystone_WhiteTiger.gltf";
}

//接地時イベント
void Allie::OnLanding()
{
	
}

//被弾時イベント
void Allie::OnDamage(uint32_t attacker_id, float hate)
{

}

//死亡時イベント
void Allie::OnDead()
{
	SquadManager::Instance().UnregisterMember(this);
}


