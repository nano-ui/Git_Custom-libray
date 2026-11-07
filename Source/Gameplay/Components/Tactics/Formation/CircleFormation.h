#pragma once

#include "FormationBase.h"

class CircleFormation :public FormationBase
{
public:
	//コンストラクタ
	CircleFormation();

	//デストラクタ
	~CircleFormation()override = default;

	//中心からの陣形座標リストを取得
	std::vector<DirectX::XMFLOAT3> CalculatePositions(size_t unit_count)override;

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//Gui登録
	void SetupInspector(GuiInspector* inspector)override;

	//半径の設定
	void SetRadius(float new_radius) { radius = new_radius; }

	//半径の取得
	float GetRadius()const { return radius; }

private:
	float radius;					//半径
};

