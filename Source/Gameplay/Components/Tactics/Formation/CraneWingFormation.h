#pragma once
#include "FormationBase.h"

class CraneWingFormation : public FormationBase
{
public:
	//コンストラクタ
	CraneWingFormation();

	//デストラクタ
	~CraneWingFormation()override = default;

	//中心からの陣形座標リストを取得
	std::vector<DirectX::XMFLOAT3> CalculatePositions(size_t unit_count)override;

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//Gui登録
	void SetupInspector(GuiInspector* inspector)override;

public:
	//左右へ広がる間隔の設定
	void SetSpacingX(float spacing) { spacing_x = spacing; }

	//左右へ広がる間隔を取得
	float GetSpacingX()const { return spacing_x; }

	//前進する前行き方向の設定
	void SetSpacingZ(float spaceing) { spacing_z = spaceing; }

	//前進する前行き方向の取得
	float GetSpacingZ()const { return spacing_z; }

private:
	float spacing_x;	//左右へ広がる間隔
	float spacing_z;	//前進する前行き方向の間隔
};

