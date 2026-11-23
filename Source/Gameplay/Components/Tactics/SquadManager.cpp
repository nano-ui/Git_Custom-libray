#include "SquadManager.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\GameObjects\GameObjectTag.h"
#include "Gameplay\GameObjects\Character\Allie.h"
#include "Gameplay\Components\AI\HateComponent.h"
#include "Gameplay\Common\Blackboard.h"
#include "Gameplay\Components\Tactics\TacticalTypes.h"
#include <Engine\Core\Log\Log.h>

//コンストラクタ
SquadManager::SquadManager()
{
	squad_blackboard = std::make_shared<Blackboard>();
	allies_hate = std::make_shared<HateComponent>();
	enemys_hate = std::make_shared<HateComponent>();
	is_initial = false;
	tactical_action = TacticalAction::None;
	squad_blackboard->BindVariable(current_tactical_action, [this]() {return static_cast<int>(tactical_action); });
}

//更新処理
void SquadManager::Update(float elapsed_time)
{
	allies_hate->Update(elapsed_time);
	enemys_hate->Update(elapsed_time);
}

//クリア
void SquadManager::Clear()
{
	members.clear();
	allies_hate->Initialize();
	enemys_hate->Initialize();
	is_initial = false;
}

//部隊リストへ登録
void SquadManager::RegisterMember(Allie* member)
{
	if (!member)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"RegisterMember: 無効なメンバーが渡されました。");
		return;
	}

	for (const auto* existing_member : members)
	{
		if (existing_member == member)
		{
			LOG_WARNING({ u8"部隊管理" }, u8"RegisterMember: 既に登録済みのメンバーです。");
			return;
		}
	}

	Blackboard* member_blackboard = member->GetBlackboard();
	if (member_blackboard)member_blackboard->SetParent(squad_blackboard);
	else LOG_WARNING({ u8"取得" }, u8"部隊のブラックボードが取得できませんでした");
	members.push_back(member);
}

//部隊リストから破棄
void SquadManager::UnregisterMember(Allie* member)
{
	if (!member)
	{
		LOG_WARNING({ u8"部隊管理" }, u8"UnregisterMember: 無効なメンバーが渡されました。");
		return;
	}

	//リスト内の該当オブジェクトの検索
	for (auto it = members.begin(); it != members.end(); ++it)
	{
		if (*it == member)
		{
			//メンバーの削除
			members.erase(it);
			return;
		}
	}
}

//目標座標を伝達
void SquadManager::CommandMode(const DirectX::XMFLOAT3& target_pos)
{
	if (members.empty())
	{
		return;
	}

	for (auto* member : members)
	{
		if (!member)
		{
			LOG_WARNING({ u8"部隊管理" }, u8"CommandMode: リスト内に無効なポインタを検出しました。");
			continue;
		}
		member->SetTargetPosition(target_pos);
		member->SetHasTarget(true);
	}
}

//陣形指定での目標座標を伝達
void SquadManager::CommandFormation(const std::vector<DirectX::XMFLOAT3>& target_positions)
{
	if (members.empty() || target_positions.empty())
	{
		LOG_WARNING({ u8"部隊管理" }, u8"部隊メンバーまたは目標座標リストが空です");
		return;
	}

	if (target_positions.size() != members.size())
	{
		LOG_WARNING({ u8"部隊管理" }, u8"部隊数と目標座標数が一致しません");
	}

	size_t loop_count = (std::min)(members.size(), target_positions.size());

	for (int i = 0; i < loop_count; ++i)
	{
		if (!members[i])
		{
			LOG_WARNING({ u8"部隊管理" }, u8"対象のメンバーがnullです");
			continue;
		}

		members[i]->SetTargetPosition(target_positions[i]);
		members[i]->SetHasTarget(true);
	}
}

//シーン開始時に初期ヘイト値を登録
void SquadManager::DistributeInitialHate(float initial_hate)
{
	if (is_initial)return;
	std::vector<uint32_t> allies_list = {};
	std::vector<uint32_t> enamys_list = {};
	//陣営ごとのID収集
	for (const auto& obj : ObjectManager::Instance().GetGameObjects())
	{
		if (!obj || !obj->IsActive())continue;
		GameObjectTag tag = obj->GetTag();
		switch (tag)
		{
		case GameObjectTag::Allies:
			allies_list.push_back(obj->GetObjectId());
			break;
		case GameObjectTag::Enemys:
			enamys_list.push_back(obj->GetObjectId());
			break;
		}
	}

	//共有ヘイトへの一斉登録
	if (allies_list.empty() || enamys_list.empty())return;
	for (int allie = 0; allie < allies_list.size(); ++allie)
	{
		enemys_hate->AddHate(allies_list[allie], initial_hate);
	}
	for (int enemy = 0; enemy < enamys_list.size();++enemy)
	{
		allies_hate->AddHate(enamys_list[enemy], initial_hate);
	}
	is_initial = true;
}

//部隊指示を設定
void SquadManager::SetTacticalAction(TacticalAction action)
{
	tactical_action = action;
}

//部隊指示の取得
TacticalAction SquadManager::GetTacticalAction() const
{
	return tactical_action;
}
