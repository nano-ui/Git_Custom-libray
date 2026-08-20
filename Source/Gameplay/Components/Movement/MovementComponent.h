#pragma once
#include "Gameplay\Components\Base\Component.h"
#include "Engine\Collision\Collider.h"

#include <memory>
#include <DirectXMath.h>

class TransformComponent;
class JsonSerializer;
class GuiInspector;

class MovementComponent :public Component
{
public:
	//コンストラクタ
	MovementComponent();

	//デストラクタ
	virtual ~MovementComponent();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//ImGuiデバッグ描画処理
	void RenderGui()override;

	//JSON保存変数登録
	void SetupSerialization(JsonSerializer* serializer);

	//inspector登録
	void SetupInspector(GuiInspector* inspector);

	//対象のTeansformComponentを指定
	void SetTransformComponent(const std::shared_ptr<TransformComponent>& transform);

	//移動方向と速度の入力設定
	void Move(float vx, float vz, float speed);

	//移動方向への旋回処理
	void Turn(float elapsed_time, float vx, float vz, float speed);

	//ジャンプ処理
	void Jump(float speed);

	//ゲッター群
	bool IsGround()const { return is_graund; }
	float GetMoveSpeed()const { return move_speed; }
	const DirectX::XMFLOAT3& GetVelocity()const { return velocity; }
	float GetMaxSpeed()const { return max_speed; }

	//セッター群
	void SetVelocity(const DirectX::XMFLOAT3& vel) { velocity = vel; }
	void SetGround(bool ground) { is_graund = ground; }

private:
	void UpdateVelocity(float elapsed_time);
	void UpdateVerticalVelocity(float elapsed_time);
	void UpdateVericalMove(float elapsed_time);
	void UpdateHorizontalVelocity(float elapsed_time);
	void UpdateHorizontalMove(float elapsed_time);

private:
	std::weak_ptr<TransformComponent> target_transform;	//対象の行列
	DirectX::XMFLOAT3 velocity;		//移動速度ベクトル
	DirectX::XMFLOAT2 move_input;	//入力方向ベクトル
	float move_speed = 0.0f;		//水平速度
	float max_speed = 5.0f;			//最大移動速度
	float acceleration = 50.0f;		//加速度
	float friction = 15.0f;			//摩擦力
	float gravity = -10.0f;			//重力
	float air_control = 0.3f;		//空中制御力
	bool is_graund = false;			//接地フラグ
};

