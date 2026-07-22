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
	animation_component = nullptr;
	root_motion_component = std::make_unique<RootMotionComponent>();
}

//デストラクタ
Character::~Character()
{
}

//初期化処理
void Character::Initialize()
{
	is_active = true;
	SetupSerialization();
	SetupBlackboard();

	if (root_motion_component && character)
	{
		root_motion_component->Initialize(character->GetGltfModelData());
	}
}

//更新処理
void Character::Update(float elapsed_time)
{
	UpdateInvincibleTimer(elapsed_time);
	if (!root_motion_component->IsEnable())
	{
		UpdateVelocity(elapsed_time);
	}

	DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMStoreFloat4(&rotation, q);

		blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"速度", move_speed);
	blackboard->SetValue(u8"接地フラグ", is_ground);

	if (state_machine_component)state_machine_component->Update(elapsed_time, blackboard.get());

	if (state_machine_component && animation_component)
	{
		std::string target_anim_name = state_machine_component->GetCurrentAnimationName(); // 最新のアニメーション名
		uint32_t current_state_id = state_machine_component->GetCurrentNodeId(); // 最新のステートID
		bool target_anim_loop = state_machine_component->GetAnimationLoop();
	
		animation_component->PlayAnimationByName(target_anim_name, current_state_id, target_anim_loop);
	}

	if (animation_component)
	{
		animation_component->Update(elapsed_time);
		UpdateRootMotion();
		root_motion_component->TraceRootMotionDebug(position);
	}

}

//描画処理
void Character::Render(ID3D11DeviceContext* context)
{
	DirectX::XMMATRIX world_matrix = GetWorldMatrix();
	DirectX::XMFLOAT4X4 transform_matrix;
	DirectX::XMStoreFloat4x4(&transform_matrix, world_matrix);
	character->Render(context, transform_matrix);
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
	serializer->RegisterVariable(u8"体力", &health, u8"基本ステータス");
	serializer->RegisterVariable(u8"攻撃力", &attack_power, u8"基本ステータス");

	//カテゴリ「移動・物理パラメータ」への登録
	serializer->RegisterVariable(u8"最高速度", &max_speed, u8"移動・物理パラメータ");
	serializer->RegisterVariable(u8"現在の移動速度", &move_speed, u8"移動・物理パラメータ");
	serializer->RegisterVariable(u8"加速度", &acceleration, u8"移動・物理パラメータ");
	serializer->RegisterVariable(u8"摩擦力", &friction, u8"移動・物理パラメータ");
	serializer->RegisterVariable(u8"重力", &gravity, u8"移動・物理パラメータ");
	serializer->RegisterVariable(u8"重量", &weight, u8"移動・物理パラメータ");

	//カテゴリ「当たり判定設定」への登録
	serializer->RegisterVariable(u8"コライダー半径", &radius, u8"当たり判定設定");
	serializer->RegisterVariable(u8"コライダー高さ", &height, u8"当たり判定設定");
	serializer->RegisterVariable(u8"コライダーY軸オフセット", &offset_y, u8"当たり判定設定");
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
	blackboard->RegisterVariable(u8"速度");
	blackboard->RegisterVariable(u8"接地フラグ");

	blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"速度", max_speed);
	blackboard->SetValue(u8"接地フラグ", is_ground);

	printf("Character: 共有ブラックボードにを登録しま。\n");
}

//アニメーション終了イベント
void Character::OnAnimationEnd(uint32_t state_key)
{
#ifdef _DEBUG
	std::cout << "Debug: Character::OnAnimationEnd - アニメーション再生終了を検知しま。StateKey: " << state_key << "\n";
#endif
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
	//回転の計算
	float rot_speed = speed * elapsed_time;
	float length = sqrtf(vx * vx + vz * vz);
	if (length < 0.001f)return;
	vx /= length;
	vz /= length;
	float frontX = sinf(angle.y);
	float frontZ = cosf(angle.y);

	//回転量の決定と適用
	float dot = (frontX * vx) + (frontZ * vz);
	float rot = 1.0f - dot;
	if (rot > rot_speed)rot = rot_speed;
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
	// コンポーネントやモデルデータが存在するか確認分岐
	if (!root_motion_component || !animation_component || !character) return;

	// 現在再生中のアニメーション名を取得して格納する
	std::string curreent_anim_name = animation_component->GetCurrentAnimationName();
	// アニメーションの現在の再生時間を取得して格納する
	float current_time = animation_component->GetCurrentAnimationTime();

	// 再生するアニメーションが切り替わったか確認分岐
	if (previous_animation_name != curreent_anim_name)
	{
		// アニメーション名からインデックス番号を取得して格納する
		int anim_index = character->GetAnimationIndex(curreent_anim_name.c_str());

		// インデックス番号が有効であるか確認分岐
		if (anim_index >= 0)
		{
			root_motion_component->OnAnimationChaanged(static_cast<size_t>(anim_index));
		}
		else
		{
			// エラーが発生する可能性のある箇所のためデバッグ出力を行う
			OutputDebugStringA("[Character Error] UpdateRootMotion: Animation index not found!\n");
		}
		previous_animation_name = curreent_anim_name;
		previous_animation_time = 0.0f;
	}

	// ステートマシンからルートモーションの有効フラグを取得して格納する
	bool is_rm_enabled = state_machine_component ? state_machine_component->IsCurrentRootMotionEnbled() : false;
	root_motion_component->SetEnable(is_rm_enabled);

	// ルートモーションが無効であるか確認分岐
	if (!root_motion_component->IsEnable()) return;

	root_motion_component->Update(current_time);

	// ルートモーション計算クラスから移動の差分ベクトルを取得して格納する
	DirectX::XMFLOAT3 delta_pos = root_motion_component->GetDeltaPosition();
	// ルートモーション計算クラスから回転の差分クォータニオンを取得して格納する
	DirectX::XMFLOAT4 delta_rot = root_motion_component->GetDeltaRotation();

	// 取得した差分移動量を計算用のベクトルとして読み込んだ
	DirectX::XMVECTOR local_translation = DirectX::XMLoadFloat3(&delta_pos);
	// 取得した差分回転量を計算用のクォータニオンとして読み込んだ
	DirectX::XMVECTOR local_rotation = DirectX::XMLoadFloat4(&delta_rot);

	// 計算対象となるルートノードのインデックスを取得して格納する
	int root_index = root_motion_component->GetTargetNodeIndex();

	// ルートノード自体のローカル回転行列を定義して初期化するための
	DirectX::XMMATRIX root_local_rotation_matrix = DirectX::XMMatrixIdentity();

	// ルートノードのインデックスが有効であるか確認分岐
	if (root_index >= 0)
	{
		// アニメーション計算後の全ノード配列の参照を格納する
		const auto& animated_nodes = character->GetAnimatedNodes();
		// インデックスが配列の範囲内にあるか確認分岐
		if (static_cast<size_t>(root_index) < animated_nodes.size())
		{
			// ルートノード自体の現在のローカル回転クォータニオンを読み込むための
			DirectX::XMVECTOR root_rot = DirectX::XMLoadFloat4(&animated_nodes.at(root_index).rotation);
			// ルートノードのローカル回転から回転行列を生成して格納する
			root_local_rotation_matrix = DirectX::XMMatrixRotationQuaternion(root_rot);
		}
	}

	// 【ここがポイント】モデル空間で歪んでいた移動ベクトルを、ルート自身の回転行列を通してキャラクターの正しい前後左右（基準軸）へ変換する
	DirectX::XMVECTOR corrected_local_translation = DirectX::XMVector3TransformNormal(local_translation, root_local_rotation_matrix);

	// キャラクター自身の現在のワールド行列を取得して格納する
	DirectX::XMMATRIX world_transform = GetWorldMatrix();
	// キャラクターの基準軸に直した移動量を、キャラクターのワールド空間の向きへ変換した
	DirectX::XMVECTOR world_translation = DirectX::XMVector3TransformNormal(corrected_local_translation, world_transform);

	// キャラクターの高さ方向の移動成分をゼロにクランプして相殺する処理
	world_translation = DirectX::XMVectorSetY(world_translation, 0.0f);

	// コライダーの移動に適用するための最終的な3次元移動数値を格納する
	DirectX::XMFLOAT3 final_movement;
	DirectX::XMFLOAT3 final_movement_temp;
	DirectX::XMStoreFloat3(&final_movement_temp, world_translation);

	// 軸がずれていた場合にゲーム上の前進・横移動として正しく適用されるようにマッピングを整理した処理
	final_movement.x = final_movement_temp.x;
	final_movement.y = final_movement_temp.y;
	final_movement.z = final_movement_temp.z;

	position.x += final_movement.x;
	position.y += final_movement.y;
	position.z += final_movement.z;

	// ルートノードのインデックスが有効であるか確認分岐
	if (root_index >= 0)
	{
		// アニメーション計算後の全ノード配列の参照を格納する
		const std::vector<GltfModelData::node>& animated_nodes = character->GetAnimatedNodes();

		// インデックスが配列の範囲内にあるか確認分岐
		if (static_cast<size_t>(root_index) < animated_nodes.size())
		{
			// モデルの初期状態におけるローカル座標を取得して格納する
			DirectX::XMFLOAT3 initial_pose_pos = root_motion_component->GetInitialLocalPosition();
			// アニメーションによって移動した現在のローカル座標を取得して格納する
			DirectX::XMFLOAT3 current_local_pos = animated_nodes.at(root_index).translation;

			// 補正を適用するためのローカル座標をコピーして格納する
			DirectX::XMFLOAT3 new_local_pos = current_local_pos;

			// モデルが勝手に移動して飛び出さないようにX・Y・Zすべての移動軸を完全に初期位置へとリセットする処理
			new_local_pos.x = initial_pose_pos.x;
			new_local_pos.y = initial_pose_pos.y;
			new_local_pos.z = initial_pose_pos.z;

			character->SetNodeTranslation(root_index, new_local_pos);
			character->RecalculateTransforms();
		}
		else
		{
			// 配列の範囲外アクセスを防ぐための境界チェックでエラーを検出したためデバッグ出力を行う
			OutputDebugStringA("[Character Error] UpdateRootMotion: root_index is out of range of animated_nodes!\n");
		}
	}
	previous_animation_time = current_time;
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
