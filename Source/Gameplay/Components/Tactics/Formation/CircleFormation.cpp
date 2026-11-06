#include "CircleFormation.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"

//ƒRƒ“ƒXƒgƒ‰ƒNƒ^
CircleFormation::CircleFormation()
{
	radius = 5.0f;
}

//’†S‚©‚ç‚Ì‘Š‘ÎÀ•WƒŠƒXƒg‚ğæ“¾
std::vector<DirectX::XMFLOAT3> CircleFormation::CalculatePositions(size_t unit_count)
{
	if (unit_count <= 0)
	{
		LOG_WARNING({ u8"İ’è" }, u8"•”‘àˆõ‚ª‚¢‚Ü‚¹‚ñ");
		return {};
	}

	if (radius <= 0.0f)
	{
		LOG_WARNING({ u8"İ’è" }, u8"–§W‚µ‚Ä‚¢‚Ü‚·");
		return {};
	}

	std::vector<DirectX::XMFLOAT3> position_list = {};
	position_list.reserve(unit_count);
	float angle_step = DirectX::XM_2PI / unit_count;
	DirectX::XMFLOAT3 target_positon = target_pos;
	for (int i = 0; i < unit_count; ++i)
	{
		float angle = i * angle_step;
		float offset_x = 0.0f;
		float offset_z = 0.0f;
		DirectX::XMScalarSinCos(&offset_x, &offset_z, angle);
		offset_x *= radius;
		offset_z *= radius;
		target_positon.x += offset_x;
		target_positon.z += offset_z;
		position_list.emplace_back(target_positon.x, 0.0f, target_positon.z);
		target_positon = target_pos;
	}
	return position_list;
}

//ƒVƒŠƒAƒ‰ƒCƒY“o˜^
void CircleFormation::SetupSerialization(JsonSerializer* serializer)
{
	FormationBase::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"‰~w‚Ì”¼Œa", &radius, SerializeTarget::Detail);
}

//Gui“o˜^
void CircleFormation::SetupInspector(GuiInspector* inspector)
{
	FormationBase::SetupInspector(inspector);
	inspector->RegisterVariable(u8"‰~w‚Ì”¼Œa", &radius, u8"‰~w");
}
