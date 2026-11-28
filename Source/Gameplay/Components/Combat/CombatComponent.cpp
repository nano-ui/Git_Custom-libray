#include "CombatComponent.h"
#include "Serialization\JsonSerializer.h"
#include <imgui.h>
#include <algorithm>

//コンストラクタ
CombatComponent::CombatComponent()
{
	SetComponentName(u8"戦闘");
	Initialize();
}

//デストラクタ
CombatComponent::~CombatComponent() = default;

//初期化処理
void CombatComponent::Initialize()
{
	attack_power = 0.0f;
}

//更新処理
void CombatComponent::Update(float elapsed_time)
{
	previous_hit_ids = std::move(current_hit_ids);
	current_hit_ids.clear();
}

//Gui描画
void CombatComponent::RenderGui()
{
	if (ImGui::CollapsingHeader(GetComponentName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat(u8"攻撃力", &attack_power);
	}
}

//JSON保存用変数登録
void CombatComponent::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"攻撃力", &attack_power, SerializeTarget::Detail);
}

//登録済みの判定か確認
bool CombatComponent::TryRegisterHit(uint32_t attack_id)
{
	const auto current_found = std::find(current_hit_ids.begin(), current_hit_ids.end(), attack_id);
	if (current_found != current_hit_ids.end())return false;
	current_hit_ids.push_back(attack_id);
	const auto previous_found = std::find(previous_hit_ids.begin(), previous_hit_ids.end(), attack_id);
	if (previous_found == previous_hit_ids.end())return true;
	return false;
}
