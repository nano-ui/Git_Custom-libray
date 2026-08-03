#include "Character.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"

#include <imgui.h>
#include <cmath>

//コンストラクタ
Character::Character()
{
	angle = { 0.0f,0.0f,0.0f };
	velocity = { 0.0f,0.0f,0.0f };
	is_ground = false;
	invincible_timer = 0.0f;
	radius = 2.0f;
	height = 4.0f;
	gravity = -10.0f;
	height = 1.0f;
	acceleration = 50.0f;
	move_vecX = 0.0f;
	move_vecZ = 0.0f;
	friction = 15.0f;
	max_speed = 5.0f;
	air_control = 0.3f;
	offset_y = 0.0f;
	weight = 10.0f;
	health = 100.0f;
	move_speed = 0.0f;

	blackboard = std::make_unique<StateBlackboard>();
	state_machine_component = std::make_unique<StateMachineComponent>();
	sequencer_component = std::make_unique<AnimationSequencerComponent>(model);
}

//デストラクタ
Character::~Character()
{
}

//初期化処理
void Character::Initialize()
{
	is_active = true;
	SetupBlackboard();
	if (sequencer_component && model)sequencer_component->Initialize(model->GetModelPath());
}

//更新処理
void Character::Update(float elapsed_time)
{
	UpdateInvincibleTimer(elapsed_time);

	if (model)model->Update(elapsed_time);

	move_speed = std::sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);

	blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"接地フラグ", is_ground);
	blackboard->SetValue("move_speed", move_speed);

	bool is_anim_finished = sequencer_component ? sequencer_component->IsAnimationFinished() : false;

	if (state_machine_component)
	{
		state_machine_component->Update(elapsed_time, blackboard.get(), is_anim_finished);

		std::string target_anim_name = state_machine_component->GetCurrentAnimationName();
		// アニメーションの変更を検知して補間切り替え命令を発行
		if (!target_anim_name.empty() && previous_animation_name != target_anim_name)
		{
			if (sequencer_component)
			{
				constexpr float default_blend_time = 0.2f; // 補間時間 (0.2秒)
				sequencer_component->ChangeAnimation(target_anim_name, default_blend_time);
			}
			previous_animation_name = target_anim_name;
		}
	}

	if (sequencer_component)
	{
		sequencer_component->Update(elapsed_time);
		current_animation_name = sequencer_component->GetCurrentAnimationName();
		current_animation_time = sequencer_component->GetCurrentSequenceTime();
	}
	else OutputDebugStringA("[Character 警告] Update: sequencer_component が nullptr のためアニメーション情報を更新できません。\n");

	//回転クォータニオンの更新
	DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMStoreFloat4(&rotation, q);

	//移動処理
	bool is_rm_enabled = state_machine_component ? state_machine_component->IsCurrentRootMotionEnbled() : false;
	debug_root_motion_enabled = is_rm_enabled;
	if (model)
	{
		model->SetRootMotionEnable(is_rm_enabled);
		if (is_rm_enabled)
		{
			UpdateRootMotion();
		}
		else
		{
			debug_root_motion_delta = { 0.0f, 0.0f, 0.0f };
			UpdateVelocity(elapsed_time);
		}
	}
	else
	{
		debug_root_motion_delta = { 0.0f, 0.0f, 0.0f };
		UpdateVelocity(elapsed_time);
	}
}

//描画処理
void Character::Render(ID3D11DeviceContext* context)
{
	DirectX::XMMATRIX world_matrix = GetWorldMatrix();
	DirectX::XMFLOAT4X4 transform_matrix;
	DirectX::XMStoreFloat4x4(&transform_matrix, world_matrix);
	model->Render(context, transform_matrix);
}

//デバッグ描画
void Character::RenderDebug(ShapeRenderer* renderer)
{

}

//をシリアライザに登録
void Character::SetupSerialization()
{
	GameObject::SetupSerialization();

	//カテゴリ「基本ステータス」への登録
	serializer->RegisterVariable(u8"体力", &health);
	serializer->RegisterVariable(u8"攻撃力", &attack_power);
	inspector->RegisterVariable(u8"体力", &health, u8"基本ステータス");
	inspector->RegisterVariable(u8"攻撃力", &attack_power, u8"基本ステータス");

	//カテゴリ「移動・物理パラメータ」への登録
	serializer->RegisterVariable(u8"最高速度", &max_speed);
	serializer->RegisterVariable(u8"加速度", &acceleration);
	serializer->RegisterVariable(u8"摩擦力", &friction);
	serializer->RegisterVariable(u8"重力", &gravity);
	serializer->RegisterVariable(u8"重量", &weight);
	serializer->RegisterVariable(u8"空中制御力", &air_control);
	inspector->RegisterVariable(u8"最高速度", &max_speed, u8"移動・物理パラメータ");
	inspector->RegisterVariable(u8"加速度", &acceleration, u8"移動・物理パラメータ");
	inspector->RegisterVariable(u8"摩擦力", &friction, u8"移動・物理パラメータ");
	inspector->RegisterVariable(u8"重力", &gravity, u8"移動・物理パラメータ");
	inspector->RegisterVariable(u8"重量", &weight, u8"移動・物理パラメータ");
	inspector->RegisterVariable(u8"空中制御力", &air_control, u8"移動・物理パラメータ");

	//カテゴリ「当たり判定設定」
	serializer->RegisterVariable(u8"コライダー半径", &radius);
	serializer->RegisterVariable(u8"コライダー高さ", &height);
	serializer->RegisterVariable(u8"コライダーY軸オフセット", &offset_y);
	inspector->RegisterVariable(u8"コライダー半径", &radius, u8"当たり判定設定");
	inspector->RegisterVariable(u8"コライダー高さ", &height, u8"当たり判定設定");
	inspector->RegisterVariable(u8"コライダーY軸オフセット", &offset_y, u8"当たり判定設定");

	//カテゴリ「デバッグモニター（物理・移動状態）」
	inspector->RegisterText(u8"ステートマシンパス", &state_machine_component->GetStateMachinePath(), u8"デバッグモニター");
	inspector->RegisterText(u8"現在の移動速度", &move_speed, u8"デバッグモニター");
	inspector->RegisterText(u8"移動速度ベクトル", &velocity, u8"デバッグモニター");
	inspector->RegisterText(u8"角度(Yaw/Pitch/Roll)", &angle, u8"デバッグモニター");
	inspector->RegisterText(u8"接地フラグ", &is_ground, u8"デバッグモニター");
	inspector->RegisterText(u8"無敵時間タイマー", &invincible_timer, u8"デバッグモニター");
	inspector->RegisterText(u8"入力ベクトルX", &move_vecX, u8"デバッグモニター");
	inspector->RegisterText(u8"入力ベクトルZ", &move_vecZ, u8"デバッグモニター");
	inspector->RegisterText(u8"現在のアニメーション", &current_animation_name, u8"デバッグモニター");
	inspector->RegisterText(u8"アニメーション再生時間", &current_animation_time, u8"デバッグモニター");
	inspector->RegisterText(u8"ルートモーション有効フラグ", &debug_root_motion_enabled, u8"デバッグモニター");
	inspector->RegisterText(u8"ルートモーション移動差分", &debug_root_motion_delta, u8"デバッグモニター");
}

//ダメージ処理
bool Character::ApplyDamage(float damage, float invincible_time)
{
	//ダメージの適用条件判定
	if (damage <= 0)return false;
	if (health <= 0)return false;
	if (invincible_timer > 0.0f)return false;

	//ダメージ適用とイベント実行
	invincible_timer = invincible_time;
	health -= damage;
	if (health <= 0)
	{
		OnDead();
	}
	else
	{
		OnDamage();
	}

	return true;
}

//ブラックボードに登録・初期化
void Character::SetupBlackboard()
{
	blackboard->RegisterVariable(u8"体力");
	blackboard->RegisterVariable("move_speed");
	blackboard->RegisterVariable(u8"接地フラグ");

	blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"接地フラグ", is_ground);
	blackboard->SetValue("move_speed", move_speed);

	printf("Character: 共有ブラックボードにを登録します。\n");
}

//移動方向の設定
void Character::Move(float elapsed_time, float vx, float vz, float speed)
{
	move_vecX = vx;
	move_vecZ = vz;
	max_speed = speed;
}

//旋回処理
void Character::Tuen(float elapsed_time, float vx, float vz, float speed)
{
	//移動入力ベクトルの長さをチェックしてゼロ除算を防止
	constexpr float min_input_length = 0.001f;
	float length = std::sqrtf(vx * vx + vz * vz);
	if (length < min_input_length)return;

	//入力ベクトル(vx, vz)から目標となるY軸回転角度(Yaw)を算出
	float target_angle = std::atan2f(vx, vz);

	//現在の角度と目標角度の差分を計算し、-π ～ +π の範囲に正規化
	float angle_diff = target_angle - angle.y;

	while (angle_diff > DirectX::XM_PI)  angle_diff -= DirectX::XM_2PI;
	while (angle_diff < -DirectX::XM_PI) angle_diff += DirectX::XM_2PI;

	//フレームレート依存を防ぐ補間速度計算と角度の更新
	float rot_speed = speed * elapsed_time;

	if (std::abs(angle_diff) <= rot_speed)
	{
		angle.y = target_angle; //差分が回転速度以下なら目標角度に合わせる
	}
	else
	{
		angle.y += (angle_diff > 0.0f ? rot_speed : -rot_speed); //差分の符号に応じて左右回転
	}

	//角度(angle.y)を -π ～ +π の範囲に正規化
	while (angle.y > DirectX::XM_PI)  angle.y -= DirectX::XM_2PI;
	while (angle.y < -DirectX::XM_PI) angle.y += DirectX::XM_2PI;

	//異常値(NaN)チェック
	if (std::isnan(angle.y))
	{
		OutputDebugStringA("[Character エラー] Tuen: angle.y に NaN が検出されたため 0.0 に補正しました。\n");
		angle.y = 0.0f;
	}
}

//ジャンプ処理
void Character::Jump(float speed)
{
	velocity.y = speed;
}

//速度と座標の更新処理
void Character::UpdateVelocity(float elapsed_time)
{
	UpdateVerticalVelocity(elapsed_time);
	UpdateHorizontalVelocity(elapsed_time);
	UpdateVerticalMove(elapsed_time);
	UpdateHorizontalMove(elapsed_time);
}

//無敵時間更新処理
void Character::UpdateInvincibleTimer(float elapsed_time)
{
	if (invincible_timer > 0.0f)
	{
		invincible_timer -= elapsed_time;
	}
}

//ルートモーション更新
void Character::UpdateRootMotion()
{
	if (!model)
	{
		OutputDebugStringA("[Character 警告] UpdateRootMotion: model が nullptr です。\n");
		return;
	}

	// Model クラスから最新のルートモーション移動差分を取得
	DirectX::XMFLOAT3 delta_pos = model->GetDeltaPosition();

	debug_root_motion_delta = delta_pos;

	// 差分が全く発生していない場合のエラー検知出力
	if (delta_pos.x == 0.0f && delta_pos.y == 0.0f && delta_pos.z == 0.0f)
	{
		//OutputDebugStringA("[Character 警告] UpdateRootMotion: 移動差分(delta_pos)が 0 です。RootMotionComponent::Update が呼ばれていない可能性があります。\n");
	}

	DirectX::XMVECTOR local_translation = DirectX::XMLoadFloat3(&delta_pos);

	// ワールド変換行列をもとにキャラクターの向きへ移動量を変換
	DirectX::XMMATRIX world_transform = GetWorldMatrix();
	DirectX::XMVECTOR world_translation = DirectX::XMVector3TransformNormal(local_translation, world_transform);
	world_translation = DirectX::XMVectorSetY(world_translation, 0.0f); // Y軸補正

	DirectX::XMFLOAT3 final_movement = {};
	DirectX::XMStoreFloat3(&final_movement, world_translation);

	// 位置座標へ加算
	position.x += final_movement.x;
	position.y += final_movement.y;
	position.z += final_movement.z;
}

//ステージとの衝突処理
void Character::ResolveStageCollision(
	const CollisionResult& result,
	CapsuleCollider& collider,
	float cap_height,
	float offset_y)
{
	//押し出しベクトルの逆算
	float push_x = result.safe_position.x - collider.start_center.x;
	float push_y = result.safe_position.y - collider.start_center.y;
	float push_z = result.safe_position.z - collider.start_center.z;

	//スロープと壁・天井の自動識別
	constexpr float SLOPE_THRESHOLD = 0.5f;
	
	if (result.hit_normal.y > SLOPE_THRESHOLD)
	{
		is_ground = true;
		if (velocity.y < 0.0f)velocity.y = 0.0f;
		float penetration_depth = std::sqrtf(push_x * push_x + push_y * push_y + push_z * push_z);
		float upward_push = penetration_depth / result.hit_normal.y;
		position.y += upward_push;
	}
	else if (result.hit_normal.y < -SLOPE_THRESHOLD)
	{
		if (velocity.y > 0.0f)velocity.y = 0.0f;
		position.y += push_y;
	}
	else
	{
		position.x += push_x;
		position.z += push_z;
		float nx = result.hit_normal.x;
		float nz = result.hit_normal.z;
		float len = std::sqrtf(nx * nx + nz * nz);
		if (len > 0.001f)
		{
			nx /= len;
			nz /= len;
			float dot = velocity.x * nx + velocity.z * nz;
			if (dot < 0.0f)
			{
				velocity.x -= dot * nx;
				velocity.z -= dot * nz;
			}
		}
	}

	//コライダー一の即時上書き
	collider.start_center = position;
	collider.start_center.y += offset_y;
	collider.end_center = position;
	collider.end_center.y += cap_height + offset_y;
}

//動的オブジェクトとの衝突処理
void Character::ResolveDynamicCollision(
	const CollisionResult& result,
	CapsuleCollider& collider,
	float cap_height,
	float offset_y)
{
	//相手のコライダー情報が存在かチェック
	if (!result.hit_collider)return;

	//互いの重さを取得
	float my_weight = collider.weight;
	float other_weight = result.hit_collider->weight;

	//押し出される割合の計算
	float push_ratio = 1.0f;
	if (my_weight > 0.0f && other_weight > 0.0f)
	{
		push_ratio = other_weight / (my_weight + other_weight);
	}
	else if (my_weight <= 0.0f && other_weight > 0.0f)
	{
		push_ratio = 0.0f;
	}
	else if (my_weight > 0.0f && other_weight <= 0.0f)
	{
		push_ratio = 1.0f;
	}
	else
	{
		push_ratio = 0.0f;
	}

	//貫通量に割合を掛けて座標を補正
	position.x += result.penetration_vector.x * push_ratio;
	position.y += result.penetration_vector.y * push_ratio;
	position.z += result.penetration_vector.z * push_ratio;

	//コライダー位置の即時上書き
	collider.start_center = position;
	collider.start_center.y += offset_y;
	collider.end_center = position;
	collider.end_center.y += cap_height + offset_y;
}

//垂直方向の移動速度更新
void Character::UpdateVerticalVelocity(float elapsed_time)
{
	velocity.y += gravity * elapsed_time;
}

//垂直方向の座標更新処理
void Character::UpdateVerticalMove(float elapsed_time)
{
	position.y += velocity.y * elapsed_time;

	//接地判定処理
	if (position.y < 0.0f)
	{
		position.y = 0.0f;
		if (!is_ground)OnLanding();
		is_ground = true;
		velocity.y = 0.0f;
	}
	else
	{
		is_ground = false;
	}

}

//水平方向の速度更新処理
void Character::UpdateHorizontalVelocity(float elapsed_time)
{
	//摩擦の計算
	float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (length > 0.0f)
	{
		float current_friction = friction * elapsed_time;
		if (!is_ground)current_friction *= air_control;
		if (length > current_friction)
		{
			float vx = velocity.x / length;
			float vz = velocity.z / length;
			velocity.x -= vx * current_friction;
			velocity.z -= vz * current_friction;
		}
		else
		{
			velocity.x = 0.0f;
			velocity.z = 0.0f;
		}
	}

	//加速の計算
	if (length <= max_speed)
	{
		float move_vec_length = sqrtf(move_vecX * move_vecX + move_vecZ * move_vecZ);
		if (move_vec_length > 0.0f)
		{
			float current_acceleration = acceleration * elapsed_time;
			if (!is_ground)current_acceleration *= air_control;
			velocity.x += move_vecX * current_acceleration;
			velocity.z += move_vecZ * current_acceleration;
			float new_length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
			if (new_length > max_speed)
			{
				float vx = velocity.x / new_length;
				float vz = velocity.z / new_length;
				velocity.x = vx * max_speed;
				velocity.z = vz * max_speed;
			}
		}
	}
	//入力リセット
	move_vecX = 0.0f;
	move_vecZ = 0.0f;
}

//水平方向の座標更新処理
void Character::UpdateHorizontalMove(float elapsed_time)
{
	position.x += velocity.x * elapsed_time;
	position.z += velocity.z * elapsed_time;
}
