#pragma once
#include "Character.h"

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
	void RenderDebug()override;

	//ImGuiデバッグ描画
	void RenderGui()override;

private:
	//入力更新処理
	void UpdateInput(float elapsed_time);

private:
	float move_speed;
};

