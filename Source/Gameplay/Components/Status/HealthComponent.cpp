#include "HealthComponent.h"
#include "Serialization\JsonSerializer.h"

#include <imgui.h>

//コンストラクタ
HealthComponent::HealthComponent()
{
	SetComponentName(u8"体力コンポーネント");
	Initialize();
}

//デストラクタ
HealthComponent::~HealthComponent() = default;

//初期化処理
void HealthComponent::Initialize()
{
	health = 100.0f;
	max_health = health;
	is_damage = false;
}

//更新処理
void HealthComponent::Update(float elapsed_time)
{
	is_damage = false;
}

//Gui描画
void HealthComponent::RenderGui()
{
	if (ImGui::CollapsingHeader(GetComponentName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat(u8"体力", &health);
		ImGui::ProgressBar(health);
		ImGui::Text(u8"最大体力", &max_health);
		ImGui::Text(u8"ダメージフラグ", &is_damage);
	}
}

//JSON保存用変数登録
void HealthComponent::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"体力", &health, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"最大体力", &max_health, SerializeTarget::Detail);
}

//被弾処理
bool HealthComponent::ApplyDamage(float damage)
{
	if (damage <= 0.0f || health <= 0.0f)
	{
		return false;
	}
	health -= damage;
	if (health < 0.0f)health = 0.0f;
	is_damage = true;
	return is_damage;
}

//体力の設定
void HealthComponent::SetHealth(float hp)
{
	health = hp;
	if (health > max_health)max_health = health;
}