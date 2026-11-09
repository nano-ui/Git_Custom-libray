#include "CraneWingFormation.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

//コンストラクタ
CraneWingFormation::CraneWingFormation()
{
	spacing_x = 1.0f;
	spacing_z = 1.0f;
}

//中心からの陣形座標リストを取得
std::vector<DirectX::XMFLOAT3> CraneWingFormation::CalculatePositions(size_t unit_count)
{
	//ガード処理
	if (unit_count <= 0)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"部隊数が0以下です", unit_count);
		return {};
	}
	if (spacing_x <= 0.0f || spacing_z <= 0.0f)
	{
		LOG_WARNING({ u8"陣形" }, u8"陣形の間隔が0以下です", spacing_x, spacing_z);
		return {};
	}

	//メモリの事前確保
	std::vector<DirectX::XMFLOAT3> position_list = {};
	position_list.reserve(unit_count);

	//先頭ユニットおよび左右斜辺の座標算出
	for (int unit = 0; unit <= unit_count - 1; ++unit)
	{
		if (unit == 0)position_list.emplace_back(target_pos);
		else
		{
			DirectX::XMFLOAT3 formation_pos = target_pos;
			int depth = (unit + 1) * 0.5f;
			float symbol = (unit % 2 != 0) ? -1.0f : 1.0f;
			formation_pos.x = formation_pos.x + (symbol * static_cast<float>(depth) * spacing_x);
			formation_pos.z = formation_pos.z + (static_cast<float>(depth) * spacing_z);
			position_list.emplace_back(formation_pos);
		}
	}

	return position_list;
}

//シリアライズ登録
void CraneWingFormation::SetupSerialization(JsonSerializer* serializer)
{
	FormationBase::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"左右へ広がる間隔", &spacing_x, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"前進する前行き方向の間隔", &spacing_z, SerializeTarget::Detail);
}

//Gui登録
void CraneWingFormation::SetupInspector(GuiInspector* inspector)
{
	FormationBase::SetupInspector(inspector);
	inspector->RegisterVariable(u8"左右へ広がる間隔", &spacing_x, u8"鶴翼の陣");
	inspector->RegisterVariable(u8"前進する前行き方向の間隔", &spacing_z, u8"鶴翼の陣");
}
