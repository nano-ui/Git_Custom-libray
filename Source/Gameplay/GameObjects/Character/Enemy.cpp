#include "Enemy.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Movement\MovementComponent.h"
#include "Gameplay\Components\Editor\AttachmentColliderComponent.h"
#include "Gameplay\Components\Movement\Move\LinearMove.h"
#include "Gameplay\Components\Movement\Move\WanderMove.h"
#include "Gameplay\Components\AI\HateComponent.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Gameplay\Components\Sensor\VisionSensorComponent.h"
#include "Gameplay\Components\AI\PerceptionComponent.h"

#include "Gameplay\GameObjects\GameObjectTag.h"

#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Core\Log\Log.h"

#include <Windows.h>
#include <Engine\Core\MacroTracy.h>

static AutoRegister<Enemy> auto_register_enemy("Enemy");

static constexpr uint32_t DUMMY_BLACKBOARD_KEY = 0;
static constexpr float DEFAULT_CHASE_SPEED = 10.0f;
static constexpr float DEFAULT_STOP_DISTANCE = 1.5f;
static constexpr float EPSILON_DISTANCE_SQ = 0.0001f;

//コンストラクタ
Enemy::Enemy()
{
	BS_PROFILE_FUNCTION();
	SetClassName("Enemy");
	chase_speed = DEFAULT_CHASE_SPEED;
	stop_distance = DEFAULT_STOP_DISTANCE;
	chase_speed = DEFAULT_CHASE_SPEED;
	linear_move = std::make_unique<LinearMove>(DUMMY_BLACKBOARD_KEY);
	wander_move = std::make_unique<WanderMove>();
	Initialize();
}

//デストラクタ
Enemy::~Enemy() = default;

//初期化処理
void Enemy::Initialize()
{
	BS_PROFILE_FUNCTION();
	//基底クラスの初期化
	Character::Initialize();
	SetupComponent();
	health = 100.0f;
	SetTag(GameObjectTag::Enemys);
	perception->SetTargetTag(GameObjectTag::Player | GameObjectTag::Allies);
}

//更新処理
void Enemy::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();
	if (is_play)
	{
		UpdateTargetFromHate();
		//UpdateChase(elapsed_time);
	}
	Character::Update(elapsed_time);
}

//デバッグ描画
void Enemy::RenderDebug(ShapeRenderer* renderer)
{
	BS_PROFILE_FUNCTION();
	Character::RenderDebug(renderer);
	wander_move->RenderDebug(renderer);
	vision_sensor->RenderDebug(renderer);
}

//シリアライズ登録
void Enemy::SetupSerialization()
{
	BS_PROFILE_FUNCTION();
	Character::SetupSerialization();
	serializer->RegisterVariable(u8"開始", &is_play, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"速度", &chase_speed, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"停止距離", &stop_distance, SerializeTarget::Detail);

	if (state_machine_component && serializer)
	{
		state_machine_component->SetupSerialization(serializer.get());
	}
}

//インスペクター登録
void Enemy::SetupInspector()
{
	BS_PROFILE_FUNCTION();
	Character::SetupInspector();
	inspector->RegisterVariable(u8"開始", &is_play);
	inspector->RegisterVariable(u8"速度", &chase_speed);
	inspector->RegisterVariable(u8"停止距離", &stop_distance);
}

// アニメーション終了イベント
void Enemy::OnAnimationEnd(uint32_t state_key)
{

}

//対象を設定
void Enemy::SetTarget(const std::shared_ptr<TransformComponent>& target)
{
	BS_PROFILE_FUNCTION();
	target_transform = target;
}

//デフォルトパスの取得
std::string Enemy::GetDefaultModelPath() const
{
	return "Data/Model/Character/Enemy/Rampage_Elemental.gltf";
}

//被弾時イベント
void Enemy::OnDamage(uint32_t attacker_id, float hate)
{
	BS_PROFILE_FUNCTION();
	hate_component->AddHate(attacker_id, hate);
}

//コンポーネント群のセットアップ
void Enemy::SetupComponent()
{
	BS_PROFILE_FUNCTION();

	Character::SetupComponent();

	hate_component = GetComponent<HateComponent>();
	if (!hate_component) hate_component = AddComponent<HateComponent>();

	vision_sensor = GetComponent<VisionSensorComponent>();
	if (!vision_sensor) vision_sensor = AddComponent<VisionSensorComponent>();
	vision_sensor->SetTransformComponent(transform_component);

	perception = GetComponent<PerceptionComponent>();
	if (!perception)perception = AddComponent<PerceptionComponent>();
	perception->SetHate(hate_component);
	perception->SetVisionSensor(vision_sensor);
}

//座標計算・移動入力の更新処理
void Enemy::UpdateChase(float elapsed_time)
{
	BS_PROFILE_FUNCTION();
	auto target = target_transform.lock();
	DirectX::XMFLOAT3 move_velocity = { 0.0f,0.0f,0.0f };
	if (!target)
	{
		move_velocity = wander_move->UpdateWander(elapsed_time, transform_component->GetPosition(), chase_speed);
	}
	else
	{
		//座標の取得
		DirectX::XMFLOAT3 current_pos = transform_component->GetPosition();
		DirectX::XMFLOAT3 target_pos = target->GetPosition();
		target_pos.y = current_pos.y;

		//距離を算出
		DirectX::XMVECTOR current_v = DirectX::XMLoadFloat3(&current_pos);
		DirectX::XMVECTOR target_v = DirectX::XMLoadFloat3(&target_pos);
		DirectX::XMVECTOR diff_v = DirectX::XMVectorSubtract(target_v, current_v);
		DirectX::XMVECTOR dist_sq_v = DirectX::XMVector3LengthSq(diff_v);

		float distance_sq = 0.0f;
		DirectX::XMStoreFloat(&distance_sq, dist_sq_v);

		float stop_distance_sq = stop_distance * stop_distance;
		if (distance_sq <= stop_distance_sq || distance_sq < EPSILON_DISTANCE_SQ)
		{
			Character::Move(elapsed_time, 0.0f, 0.0f);
			return;
		}

		//移動速度を算出
		move_velocity = linear_move->UpdateLiner(current_pos, target_pos, chase_speed);
	}
	float length = std::sqrtf((move_velocity.x * move_velocity.x) + (move_velocity.z * move_velocity.z));
	if (length < EPSILON_DISTANCE_SQ)
	{
		Character::Move(elapsed_time, 0.0f, 0.0f);
		return;
	}

	float dir_x = move_velocity.x / length;
	float dir_z = move_velocity.z / length;

	//実際に移動
	Character::Move(elapsed_time, dir_x, dir_z);
	Character::Tuen(elapsed_time, dir_x, dir_z);
}

//注目対象更新処理
void Enemy::UpdateTargetFromHate()
{
	BS_PROFILE_FUNCTION();
	if (!hate_component)
	{
		LOG_WARNING({ "Enemy","Target" }, u8"hate_componentがnullです");
		return;
	}

	//対象IDの取得
	bool is_found = false;
	uint32_t target_id = hate_component->GetTopHateTargetId();
	if (!hate_component->HasTarget())
	{
		target_transform.reset();
		return;
	}
	
	//対象の検索
	for (const auto& obj : ObjectManager::Instance().GetGameObjects())
	{
		if (!obj || !obj->IsActive())
		{
			continue;
		}

		if (target_id == obj->GetObjectId())
		{
			auto transform = obj->GetComponent<TransformComponent>();
			if (transform)
			{
				target_transform = transform;
				is_found = true;
				break;
			}
			else
			{
				LOG_WARNING({ "Enemy","Target" }, u8"対象の TransformComponent が取得できませんでした");
			}
		}
	}
	if (!is_found)
	{
		target_transform.reset();
	}
}
