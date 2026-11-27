#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

class Allie;
class HateComponent;
class Blackboard;
enum class TacticalAction;

class SquadManager
{
public:
	//シングルトンの取得
	static SquadManager& Instance()
	{
		static SquadManager instance;
		return instance;
	}

	//コンストラクタ
	SquadManager();

	//更新処理
	void Update(float elapsed_time);

	//クリア
	void Clear();

	//部隊リストへ登録
	void RegisterMember(Allie* member);

	//部隊リストから破棄
	void UnregisterMember(Allie* member);

	//目標座標を伝達
	void CommandMode(const DirectX::XMFLOAT3& target_pos);

	//陣形指定での目標座標を伝達
	void CommandFormation(const std::vector<DirectX::XMFLOAT3>& target_positions);

	//シーン開始時に初期ヘイト値を登録
	void DistributeInitialHate(float initial_hate);

	//移動目標を持つメンバーが存在するか確認
	bool HasActiveTarget()const;

public:
	//味方の共有ヘイトコンポーネントを取得
	std::shared_ptr<HateComponent> GetAlliesHate()const { return allies_hate; }

	//敵の共通ヘイトコンポーネントを取得
	std::shared_ptr<HateComponent> GetEnemysHate()const { return enemys_hate; }

	//ブラックボードの取得
	std::shared_ptr<Blackboard> GetSquadBlackboard()const { return squad_blackboard; }

	//部隊数を取得
	size_t GetMenberCount()const { return members.size(); }

	//部隊指示を設定
	void SetTacticalAction(TacticalAction action);

	//部隊指示の取得
	TacticalAction GetTacticalAction()const;

private:

	//デストラクタ
	~SquadManager() = default;

private:
	std::shared_ptr<HateComponent> allies_hate;		//味方のヘイト
	std::shared_ptr<HateComponent> enemys_hate;		//敵のヘイト
	std::shared_ptr<Blackboard> squad_blackboard;	//部隊全体のブラックボード

private:
	std::vector<Allie*> members;	//部隊リスト
	bool is_initial;
	TacticalAction tactical_action;	//部隊指示
};

