#pragma once
#include "Gameplay\Components\Base\Component.h"

#include <vector>
#include <cstdint>

class JsonSerializer;

class CombatComponent :public Component
{
public:
	//コンストラクタ
	CombatComponent();

	//デストラクタ
	~CombatComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//Gui描画
	void RenderGui()override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

public:
	//登録済みの判定か確認
	bool TryRegisterHit(uint32_t attack_id);

public:
	//攻撃力の取得
	float GetAttackPower()const { return attack_power; }

	//攻撃力の設定
	void SetAttackPower(float power) { attack_power = power; }

private:
	float attack_power;						//攻撃力
	std::vector<uint32_t> current_hit_ids;	//今回の接触IDリスト
	std::vector<uint32_t> previous_hit_ids;	//前回の接触IDリスト
};

