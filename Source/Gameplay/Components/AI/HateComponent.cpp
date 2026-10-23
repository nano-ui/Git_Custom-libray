#include "HateComponent.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

#include <Windows.h>
#include <imgui.h>
#include <Engine\Core\MacroTracy.h>

//コンストラクタ
HateComponent::HateComponent()
	:decay_rate(DEFAULT_DECAY_RATE)
	, debug_top_hate_id(NOT_TARGET_ID)
	, debug_top_hate_value(0.0f)
{
	BS_PROFILE_FUNCTION();

	SetComponentName(u8"ヘイト管理");
}

//デストラクタ
HateComponent::~HateComponent() = default;

//初期化処理
void HateComponent::Initialize()
{
	BS_PROFILE_FUNCTION();

	Component::Initialize();
	hate_table.clear();
}

//更新処理
void HateComponent::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

	//有効フラグと引数の検証
	if (!is_active)return;

	if (elapsed_time <= 0.0f)
	{
		OutputDebugStringA("[HateComponent::Update] 経過時間が0以下です。\n");
		return;
	}

	if (!HasTarget())return;

	//減衰量の算出
	float decay_amount = 0.0f;
	decay_amount = decay_rate * elapsed_time;

	//全てのヘイト値の減衰
	for (auto it = hate_table.begin(); it != hate_table.end();)
	{
		it->second -= decay_amount;
		if (it->second <= 0.0f)
		{
			//対象IDをヘイトテーブルから削除
			it = hate_table.erase(it);
		}
		else ++it;
	}

	//監視数値の同期
	debug_top_hate_id = GetTopHateTargetId();
}

//デバッグ描画処理
void HateComponent::RenderGui()
{
	BS_PROFILE_FUNCTION();

	Component::RenderGui();

	if(ImGui::TreeNode(u8"ヘイトリスト一覧"))
	{
		if (hate_table.empty())
		{
			ImGui::Text(u8"対象無し");
		}
		else
		{
			for (const auto& [id, hate] : hate_table)
			{
				ImGui::Text(u8"ID: %u | ヘイト値: %.2f", id, hate);
			}
		}
		ImGui::TreePop();
	}
}

//JSON保存用変数登録
void HateComponent::SetupSerialization(JsonSerializer* serializer)
{
	BS_PROFILE_FUNCTION();

	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"減衰量", &decay_rate, SerializeTarget::Detail);
}

//GuiInspector専用のインスペクター項目登録
void HateComponent::SetupInspector(GuiInspector* inspector)
{
	BS_PROFILE_FUNCTION();

	Component::SetupInspector(inspector);
	inspector->RegisterVariable(u8"減衰量", &decay_rate, GetComponentName());
	inspector->RegisterText(u8"最大ヘイトID", &debug_top_hate_id, GetComponentName());
	inspector->RegisterText(u8"最大ヘイト値", &debug_top_hate_value, GetComponentName());
}

//ヘイト加算
void HateComponent::AddHate(uint32_t target_id, float hate)
{
	BS_PROFILE_FUNCTION();

	if (target_id == NOT_TARGET_ID)
	{
		OutputDebugStringA("[HateComponent::AddHate] 対象IDが無効な値です。\n");
		return;
	}
	if (hate <= 0.0f)
	{
		OutputDebugStringA("[HateComponent::AddHate] 加算するヘイト値が0以下です。\n");
		return;
	}
	hate_table[target_id] += hate;
}

//最大ヘイト値ID取得
uint32_t HateComponent::GetTopHateTargetId()
{
	BS_PROFILE_FUNCTION();

	//存在確認
	if (!HasTarget())
	{
		OutputDebugStringA("[HateComponent::GetTopHateTargetId] ヘイト対象が存在しません。\n");
		return NOT_TARGET_ID;
	}

	//ローカル変数の定義
	float max_hate = 0.0f;					//最大ヘイト値
	uint32_t top_hate_id = NOT_TARGET_ID;	//最大ヘイトID

	//最大ヘイト値と紐づいているIDを探索
	for (const auto& [id, hate] : hate_table)
	{
		if (max_hate < hate)
		{
			max_hate = hate;
			top_hate_id = id;
		}
	}

	debug_top_hate_value = max_hate;

	//最大ヘイト値IDを返す
	return top_hate_id;
}

//ヘイト対象が存在するか判定
bool HateComponent::HasTarget() const
{
	BS_PROFILE_FUNCTION();

	return (!hate_table.empty());
}
