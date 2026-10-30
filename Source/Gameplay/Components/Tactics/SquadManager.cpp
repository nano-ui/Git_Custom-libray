#include "SquadManager.h"
#include "Gameplay\GameObjects\Character\Allie.h"
#include <Engine\Core\Log\Log.h>

//クリア
void SquadManager::Clear()
{
	members.clear();
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
