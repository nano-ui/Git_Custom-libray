#pragma once

#include "../GameObjects/GameObject.h"

#include <memory>
#include <vector>
#include <DirectXMath.h>

class Model;
class SpaceDivisionCast;

class Stage : public GameObject
{
public:
	//コンストラクタ
	Stage();

	//デストラクタ
	~Stage();

	//初期化
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(ID3D11DeviceContext* context)override;

	//デバッグ描画
	//void RenderDebug()override;

	//ImGuiデバッグ描画
	void RenderGui()override;

	//当たり判定用の空間分割キャストオブジェクトを取得
	SpaceDivisionCast* GetSpaceDivisionCast();

private:
	//空間分割データの構築処理
	void BuildCollisionData();

private:
	std::unique_ptr<Model> stage_model;	//ステージ描画用モデル
	std::unique_ptr<SpaceDivisionCast> space_division_cast;	//当たり判定空間分割クラスを管理
};

