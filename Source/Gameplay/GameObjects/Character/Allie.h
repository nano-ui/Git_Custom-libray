#pragma once
#include "Character.h"

#include <DirectXMath.h>

//味方クラス
class Allie : public Character
{
public:
	//コンストラクタ
	Allie();

	//デストラクタ
	~Allie()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//衝突処理
	void OnCollisionHit(const CollisionResult& result)override;

	//描画処理
	void Render(ID3D11DeviceContext* context)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	// シリアライズ登録
	void SetupSerialization() override;

	//インスペクター登録
	void SetupInspector() override;

	//コンポーネントの設定
	void SetupComponent()override;

public:
	//目標座標の設定
	void SetTargetPosition(DirectX::XMFLOAT3 pos) { target_position = pos; }

	//目標座標の取得
	DirectX::XMFLOAT3 GetTargetPosition()const { return target_position; }

	//目標フラグの設定
	void SetHasTarget(bool target) { has_target = target; }

	//目標フラグの取得
	bool GetHasTarget()const { return has_target; }

protected:
	//デフォルトパスの取得
	std::string GetDefaultModelPath()const override;

private:
	DirectX::XMFLOAT3 target_position;	//目標座標
	bool has_target;					//目標フラグ
};

