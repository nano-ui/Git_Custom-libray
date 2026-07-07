#pragma once

// インクルード
#include "../Gameplay/GameObjects/GameObject.h"
#include "../Engine/Collision/Collider.h"
#include "Gameplay\Components\Animation\RootMotionComponent.h"
#include "Gameplay\Components\Animation\AnimationComponent.h"
#include <memory>
#include <unordered_map>

// クラス宣言
class StateBlackboard;
class StateMachineComponent;
class CharacterMovementComponent;

// キャラクターの基底となるゲームオブジェクトクラス
class Character : public GameObject, public IAnimationListener
{
public:
	// コンストラクタ
	Character();

	// デストラクタ
	virtual ~Character();

	// 初期化処理
	void Initialize()override;

	// 更新処理
	void Update(float elapsed_time)override;

	// 描画処理
	void Render(ID3D11DeviceContext* context)override;

	// デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	// シリアライザへの登録
	void SetupSerialization()override;

	// ダメージ適用処理
	bool ApplyDamage(float damage, float invincible_time);

	// ブラックボードの取得
	StateBlackboard* GetBlackboard()const { return blackboard.get(); }

	// ブラックボードの登録・初期化
	virtual void SetupBlackboard();

	// アニメーション終了イベント
	virtual void OnAnimationEnd(uint32_t stake_key);

	// アニメーションコンポーネントの取得
	AnimationComponent* GetAnimationComponent()const { return animation_component.get(); }

	// ルートモーションコンポーネントの取得
	RootMotionComponent* GetRootMotionComponent()const { return root_motion_component.get(); }

	// ステートマシンコンポーネントの取得
	StateMachineComponent* GetStateMachineComponent()const { return state_machine_component.get(); }

	// 移動更新コンポーネントの取得
	CharacterMovementComponent* GetMovementComponent()const { return movement_component.get(); }

protected:
	// 移動方向の設定
	void Move(float elapsed_time, float vx, float vz, float speed);

	// ジャンプ処理
	void Jump(float speed);

	// 無敵時間の更新処理
	void UpdateInvincibleTimer(float elapsed_time);

	// ルートモーションの更新処理
	void UpdateRootMotion();

	// 接地時イベント
	virtual void OnLanding() {}

	// 被弾時イベント
	virtual void OnDamage() {}

	// 死亡時イベント
	virtual void OnDead() {}

	// ステージとの衝突応答処理
	void ResolveStageCollision(const CollisionResult& result, CapsuleCollider& collider, float cap_height, float offset_y);

	// 動的オブジェクトとの衝突応答処理
	void ResolveDynamicCollision(const CollisionResult& result, CapsuleCollider& collider, float cap_height, float offset_y);

protected:
	std::shared_ptr<Model> character;	// キャラクターモデル
	std::unique_ptr<StateBlackboard> blackboard; // 共有ブラックボード
	std::unique_ptr<AnimationComponent> animation_component;	// アニメーション制御コンポーネント
	std::unique_ptr<RootMotionComponent> root_motion_component;	// ルートモーション制御コンポーネント
	std::unique_ptr<StateMachineComponent> state_machine_component;	// ステートマシン制御コンポーネント
	std::unique_ptr<CharacterMovementComponent> movement_component; // 移動・物理制御コンポーネント

	std::string previous_animation_name = "";	// 前回のアニメーション名
	float previous_animation_time = 0.0f;		// 前回の再生時間
	DirectX::XMFLOAT3 angle;	// キャラクターの回転角度
	float radius;				// 当たり判定のコライダー半径
	float height;				// 当たり判定のコライダー高さ
	float invincible_timer;		// 被弾後の無敵時間タイマー
	float attack_power;			// 攻撃力ステータス
	float offset_y;				// 当たり判定のY軸オフセット
	float weight;				// キャラクターの重量重量
	float health;				// 生命力・体力ステータス
	bool is_ground;
};