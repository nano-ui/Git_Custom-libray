#include "RectangleFormation.h"
#include "Editor\Inspector\GuiInspector.h"
#include "Serialization\JsonSerializer.h"

//コンストラクタ
RectangleFormation::RectangleFormation()
{
	columns = 4;
	spacing_x = 1.5f;
	spacing_z = 1.5f;
}

//中心からの陣形座標リストを取得
std::vector<DirectX::XMFLOAT3> RectangleFormation::CalculatePositions(size_t unit_count)
{
	//ガード処理
	if (unit_count <= 0)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"全体のユニット数が0以下です", unit_count);
		return {};
	}
	if (columns <= 0)
	{
		LOG_WARNING({ u8"陣形" }, u8"一列内の最大ユニット数が0以下です", columns);
		return {};
	}
	if (spacing_x <= 0.0f || spacing_z <= 0.0f)
	{
		LOG_WARNING({ u8"陣形" }, u8"陣形の間隔が0以下です", spacing_x, spacing_z);
		return {};
	}

	//メモリ確保
	std::vector<DirectX::XMFLOAT3> position_list = {};
	position_list.reserve(unit_count);

	//陣形座標リストの算出
	for (size_t unit = 0; unit <= unit_count - 1; ++unit)
	{
		DirectX::XMFLOAT3 formation_pos = target_pos;
		int row = unit / columns;
		int col = unit % columns;
		float center_offset = (columns - 1) * 0.5f;
		formation_pos.x = formation_pos.x + (static_cast<float>(col) - center_offset) * spacing_x;
		formation_pos.z = formation_pos.z - (static_cast<float>(row) * spacing_z);
		position_list.emplace_back(formation_pos);
	}

	return position_list;
}

//シリアライズ登録
void RectangleFormation::SetupSerialization(JsonSerializer* serializer)
{
	FormationBase::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"一列の最大ユニット数", &columns, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"横の列の間隔", &spacing_x, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"縦の列の間隔", &spacing_z, SerializeTarget::Detail);
}

//Gui登録
void RectangleFormation::SetupInspector(GuiInspector* inspector)
{
	FormationBase::SetupInspector(inspector);
	inspector->RegisterVariable(u8"一列の最大ユニット数", &columns, u8"長方形隊列");
	inspector->RegisterVariable(u8"横の列の間隔", &spacing_x, u8"長方形隊列");
	inspector->RegisterVariable(u8"縦の列の間隔", &spacing_z, u8"長方形隊列");
}
