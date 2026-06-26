#pragma once

#include "../Gameplay/GameObjects/GameObject.h"
#include "../Engine/Collision/Collider.h"
#include "../Gameplay/Components/AnimationComponent.h"

#include <memory>
#include <unordered_map>

class StateBlackboard;
class StateMachineComponent;

class Character : public GameObject, public IAnimationListener
{
public:
	//コンストラクタ
	Character();

	//デストラクタ
	virtual ~Character();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(ID3D11DeviceContext* context)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//変数をシリアライザに登録
	void SetupSerialization()override;

	//ダメージ処理
	bool ApplyDamage(float damage, float invincible_time);

	//ブラックボードを取得
	StateBlackboard* GetBlackboard()const { return blackboard.get(); }

	//ブラックボードに登録・初期化
	virtual void SetupBlackboard();

	//アニメーション終了イベント
	virtual void OnAnimationEnd(uint32_t stake_key);

	//アニメーションコンポーネント取得
	AnimationComponent* GetAnimationComponent()const { return animation_component.get(); }

	//ステートマシンコンポーネントクラス取得
	StateMachineComponent* GetStateMachineComponent()const { return state_machine_component.get(); }

protected:
	//移動方向の設定
	void Move(float elapsed_time, float vx, float vz, float speed);

	//旋回処理
	void Tuen(float elapsed_time, float vx, float vz, float speed);

	//ジャンプ処理
	void Jump(float speed);

	//速度と座標の更新処理
	void UpdateVelocity(float elapsed_time);

	//無敵時間更新処理
	void UpdateInvincibleTimer(float elapsed_time);

	//接地時イベント
	virtual void OnLanding() {}

	//被弾時イベント
	virtual void OnDamage() {}

	//死亡時イベント
	virtual void OnDead() {}

	//ステージとの衝突処理
	void ResolveStageCollision(const CollisionResult& result, CapsuleCollider& collider, float cap_height, float offset_y);

	//動的オブジェクトとの衝突処理
	void ResolveDynamicCollision(const CollisionResult& result, CapsuleCollider& collider, float cap_height, float offset_y);

private:
	//垂直方向の移動速度更新
	void UpdateVerticalVelocity(float elapsed_time);

	//垂直方向の座標更新処理
	void UpdateVerticalMove(float elapsed_time);

	//水平方向の速度更新処理
	void UpdateHorizontalVelocity(float elapsed_time);

	//水平方向の座標更新処理
	void UpdateHorizontalMove(float elapsed_time);

protected:
	std::shared_ptr<Model> character;	//キャラクターモデル
	std::unique_ptr<StateBlackboard> blackboard;
	std::unique_ptr<AnimationComponent> animation_component;	//アニメーション制御
	std::unique_ptr<StateMachineComponent> state_machine_component;	//ステートマシン制御

	DirectX::XMFLOAT3 angle;	//角度
	float radius;				//半径
	float gravity;				//重力
	DirectX::XMFLOAT3 velocity;	//移動速度ベクトル
	float move_vecX;			//移動ベクトルX
	float move_vecZ;			//移動ベクトルZ
	bool is_ground;				//接地判定フラグ
	float height;				//高さ
	float invincible_timer;		//無敵時間
	float acceleration;			//加速度
	float max_speed;			//最大移動速度
	float attack_power;			//攻撃力
	float friction;				//摩擦力
	float air_control;			//空中での制御力
	float offset_y;				//当たり判定のY軸オフセット
	float weight;				//キャラクターの重さ
	float health;				//生命力	
	float move_speed;			//移動速度
};

