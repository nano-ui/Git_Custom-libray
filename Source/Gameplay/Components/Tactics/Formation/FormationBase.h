#pragma once

#include <vector>
#include <cstddef>
#include <DirectXMath.h>

#include "Engine\Core\Log\Log.h"

class JsonSerializer;
class GuiInspector;

class FormationBase
{
public:
	//デストラクタ
	virtual ~FormationBase() = default;

	//中心からの相対座標リストを取得
	virtual std::vector<DirectX::XMFLOAT3> CalculatePositions(size_t unit_count) = 0;

	//シリアライズ登録
	virtual void SetupSerialization(JsonSerializer* serializer)
	{
		if (!serializer)LOG_WARNING({ u8"設定" }, u8"引数がnullです");
		return;
	}

	//Gui登録
	virtual void SetupInspector(GuiInspector* inspector)
	{
		if (!inspector)LOG_WARNING({ u8"設定" }, u8"引数がnullです");
		return;
	}

	//対象座標を設定
	void SetTargetPos(DirectX::XMFLOAT3 pos) { target_pos = pos; }

	//対象座標を取得
	DirectX::XMFLOAT3 GetTargetPos()const { return target_pos; }

protected:
	DirectX::XMFLOAT3 target_pos = { 0.0f,0.0f,0.0f };	//対象座標
};

