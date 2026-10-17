#pragma once
#include "Character.h"
#include "Engine/Collision/Collider.h"

class CapsuleColliderComponent;

class Player : public Character
{
public:
	//コンストラクタ
	Player();

	//デストラクタ
	~Player();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//シリアライザに登録
	void SetupSerialization() override;

	//インスペクター登録
	void SetupInspector()override;

	//アニメーション終了イベント
	void OnAnimationEnd(uint32_t state_key)override;

protected:
	//デフォルトパスの取得
	std::string GetDefaultModelPath() const override;

private:
	//入力更新処理
	void UpdateInput(float elapsed_time);
};

