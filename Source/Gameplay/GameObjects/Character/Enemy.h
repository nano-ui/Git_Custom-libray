#pragma once

#include "Gameplay\GameObjects\Character\Character.h"
#include "Engine/Collision/Collider.h"

#include <memory>

class CollisionSphere;
class CapsuleColliderComponent;
class BoneCapsuleColliderComponent;
class AttachmentColliderComponent;

class Enemy :public Character
{
public:
	//コンストラクタ
	Enemy();

	//デストラクタ
	~Enemy()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//シリアライズ登録
	void SetupSerialization()override;

	//インスペクター登録
	void SetupInspector()override;

	// アニメーション終了イベント
	void OnAnimationEnd(uint32_t state_key) override;

protected:
	//デフォルトパスの取得
	std::string GetDefaultModelPath() const override;

private:
	//コンポーネント群のセットアップ
	void SetupComponent();

private:

};

