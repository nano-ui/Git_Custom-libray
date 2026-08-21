#pragma once

#include "Gameplay/GameObjects/GameObject.h"
#include "Engine/Collision/Collider.h"
#include "Gameplay\Components\Animation\RootMotionComponent.h"
#include "Gameplay\Components\Animation\AnimationComponent.h"
#include "Gameplay\Components\Animation\AnimationSequencerComponent.h"

#include <memory>
#include <unordered_map>

class StateBlackboard;
class StateMachineComponent;
class TransformComponent;
class ModelComponent;
class MovementComponent;

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

	// シリアライズ・インスペクター登録
	void SetupSerialization() override;
	void SetupInspector() override;

	//ダメージ処理
	bool ApplyDamage(float damage, float invincible_time);

	//ブラックボードを取得
	StateBlackboard* GetBlackboard()const { return blackboard.get(); }

	//ブラックボードに登録・初期化
	virtual void SetupBlackboard();

	//ステートマシンコンポーネントクラス取得
	StateMachineComponent* GetStateMachineComponent()const { return state_machine_component.get(); }

protected:
	//移動方向の設定
	void Move(float elapsed_time, float vx, float vz, float speed);

	//旋回処理
	void Tuen(float elapsed_time, float vx, float vz, float speed);

	//ジャンプ処理
	void Jump(float speed);

	//無敵時間更新処理
	void UpdateInvincibleTimer(float elapsed_time);

	//ルートモーション更新
	void UpdateRootMotion();

	//接地時イベント
	virtual void OnLanding() {}

	//被弾時イベント
	virtual void OnDamage() {}

	//死亡時イベント
	virtual void OnDead() {}

protected:
	std::unique_ptr<StateBlackboard> blackboard;
	std::unique_ptr<StateMachineComponent> state_machine_component;	//ステートマシン制御
	std::unique_ptr<AnimationSequencerComponent> sequencer_component;	//アニメーションシーケンサ
	std::unique_ptr<RootMotionComponent> root_motion_component;        //ルートモーション計算コンポーネント

	std::shared_ptr<TransformComponent> transform_component;//行列コンポーネント
	std::shared_ptr<ModelComponent> model_component;		//モデルコンポーネント
	std::shared_ptr<MovementComponent> movement_component;	//移動コンポーネント

	std::string current_animation_name = "";	//現在のアニメーション名
	float current_animation_time = 0.0f;		//現在のアニメーション再生時間
	std::string previous_animation_name = "";	//前回のアニメーション名
	float previous_animation_time = 0.0f;		//前回の再生時間

	bool debug_root_motion_enabled = false;		//ルートモーションの有効状態監視用
	DirectX::XMFLOAT3 debug_root_motion_delta = { 0.0f, 0.0f, 0.0f }; //ルートモーション移動差分監視用

	float invincible_timer;		//無敵時間
	float attack_power;			//攻撃力
	float health;				//生命力	
};

