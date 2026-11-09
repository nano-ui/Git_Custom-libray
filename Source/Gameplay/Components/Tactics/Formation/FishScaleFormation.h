#pragma once

#include "FormationBase.h"

//魚鱗の陣
class FishScaleFormation : public FormationBase
{
public:
	//コンストラクタ
	FishScaleFormation();

	//デストラクタ
	~FishScaleFormation()override = default;

	//中心からの陣形座標リストを取得
	std::vector<DirectX::XMFLOAT3> CalculatePositions(size_t unit_count)override;

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//Gui登録
	void SetupInspector(GuiInspector* inspector)override;

public:
	//最前列の兵士数を設定
	void SetStartUnits(int units) { start_units = units; }

	//最前列の兵士数を取得
	int GetStartUnits()const { return start_units; }

	//各列ごとに増やす兵士数を設定
	void SetStepUnits(int step) { step_units = step; }

	//各列ごとに増やす兵士数を取得
	int GetStepUnits()const { return start_units; }

	//横の列の間隔を設定
	void SetSpacingX(float spacing) { spacing_x = spacing; }

	//横の列の間隔を取得
	float GetSpscingX()const { return spacing_x; }

	//前後の列の間隔を設定
	void SetSpacingZ(float spacing) { spacing_z = spacing; }

	//前後の列の間隔を取得
	float GetSpacingZ()const { return spacing_z; }

private:
	int start_units;	//最前列の兵士数
	int step_units;		//各列ごとに増える兵士数
	float spacing_x;	//横の列の間隔
	float spacing_z;	//前後の列の間隔
};

