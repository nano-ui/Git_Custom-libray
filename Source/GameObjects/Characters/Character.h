#pragma once

#include "../GameObjects/GameObject.h"
#include "../Collision/Collider.h"

class Character : public GameObject
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

	//影の書き込みパス専用の描画処理
	void RenderCaster(ID3D11DeviceContext* context) override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//ImGui表示
	void RenderGui()override;

	//ダメージ処理
	bool ApplyDamage(float damage, float invincible_time);

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
	std::unique_ptr<Model> character;	//キャラクターモデル

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
};

