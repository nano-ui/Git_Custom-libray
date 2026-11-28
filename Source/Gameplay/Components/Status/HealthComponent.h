#pragma once
#include "Gameplay\Components\Base\Component.h"

class JsonSerializer;

class HealthComponent : public Component
{
public:
	//コンストラクタ
	HealthComponent();

	//デストラクタ
	~HealthComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//Gui描画
	void RenderGui()override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

public:
	//被弾処理
	bool ApplyDamage(float damage);

public:
	//体力の取得
	float GetHealth()const { return health; }

	//体力の設定
	void SetHealth(float hp);

	//最大体力の取得
	float GetMaxHealth()const { return max_health; }

	//最大体力の設定
	void SetMaxHealth(float hp) { max_health = hp; }

	//被弾フラグの取得
	bool GetIsDamage()const { return is_damage; }

	//被弾フラグの設定
	void SetIsDamage(bool flag) { is_damage = flag; }

private:
	float health;			//体力
	float max_health;		//最大体力
	bool is_damage;			//被弾フラグ
};

