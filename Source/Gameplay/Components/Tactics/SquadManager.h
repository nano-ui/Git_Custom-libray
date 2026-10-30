#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

class Allie;

class SquadManager
{
public:
	//シングルトンの取得
	static SquadManager& Instance()
	{
		static SquadManager instance;
		return instance;
	}

	//クリア
	void Clear();

	//部隊リストへ登録
	void RegisterMember(Allie* member);

	//部隊リストから破棄
	void UnregisterMember(Allie* member);

	//目標座標を伝達
	void CommandMode(const DirectX::XMFLOAT3& target_pos);

private:
	//コンストラクタ
	SquadManager() = default;

	//デストラクタ
	~SquadManager() = default;

private:
	std::vector<Allie*> members;	//部隊リスト
};

