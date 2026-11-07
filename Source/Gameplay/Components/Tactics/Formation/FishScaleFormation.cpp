#include "FishScaleFormation.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

//コンストラクタ
FishScaleFormation::FishScaleFormation()
{
	start_units = 2;
	step_units = 2;
	spacing_x = 1.0f;
	spacing_z = 2.0f;
}

//中心からの陣形座標リストを取得
std::vector<DirectX::XMFLOAT3> FishScaleFormation::CalculatePositions(size_t unit_count)
{
	//ガード処理
	if (unit_count <= 0)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"部隊数が0です", unit_count);
		return {};
	}

	if (start_units <= 0 || step_units <= 0)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"各列ごとのユニット数が0です", start_units, step_units);
		return {};
	}

	if (spacing_x <= 0.0f || spacing_z <= 0.0f)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"列の間隔が空いていません", spacing_x, spacing_z);
		return {};
	}

	//メモリ確保
	std::vector<DirectX::XMFLOAT3> position_list = {};
	position_list.reserve(unit_count);
	int remaining_units = static_cast<int>(unit_count);	//残りのユニット数

	//列ごとの人数割り当てと座標算出
	for (int row = 0; remaining_units > 0; ++row)
	{
		int current_row_units = start_units + row * step_units;
		if (current_row_units > remaining_units)current_row_units = remaining_units;
		float offset_z = -static_cast<float>(row) * spacing_z;
		DirectX::XMFLOAT3 formation_pos = target_pos;
		for (int row_unit = 0; row_unit < current_row_units; ++row_unit)
		{
			float offset_x = (static_cast<float>(row_unit) - (static_cast<float>(current_row_units - 1) * 0.5f)) * spacing_x;
			formation_pos.x += offset_x;
			formation_pos.z += offset_z;
			position_list.emplace_back(formation_pos);
			formation_pos = target_pos;
		}
		remaining_units -= current_row_units;
	}
	return position_list;
}

//シリアライズ登録
void FishScaleFormation::SetupSerialization(JsonSerializer* serializer)
{
	FormationBase::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"最前列の兵士数", &start_units, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"各列ごとに増える兵士数", &step_units, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"横の列の間隔", &spacing_x, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"前後の列の間隔", &spacing_z, SerializeTarget::Detail);
}

//Gui登録
void FishScaleFormation::SetupInspector(GuiInspector* inspector)
{
	FormationBase::SetupInspector(inspector);
	inspector->RegisterVariable(u8"最前列の兵士数", &start_units, u8"魚鱗の陣");
	inspector->RegisterVariable(u8"各列ごとに増える兵士数", &step_units, u8"魚鱗の陣");
	inspector->RegisterVariable(u8"横の列の間隔", &spacing_x, u8"魚鱗の陣");
	inspector->RegisterVariable(u8"前後の列の間隔", &spacing_z, u8"魚鱗の陣");
}
