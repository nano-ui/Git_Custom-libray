#pragma once
#include "Gameplay\Components\Base\Component.h"

#include <memory>
#include <vector>

class GameObject;

class SituationEvaluator : public Component 
{
public:
	//コンストラクタ
	SituationEvaluator();

	//デストラクタ
	~SituationEvaluator()override;

	//初期化
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override {}

	//GameObjectを利用したスコア計算
	void ObjectsScoreCalculation(const std::vector<GameObject*>& game_objects);

public:
	//平均HPスコア取得
	float GetAverageHpScore()const { return average_hp_score; }

	//瀕死スコア取得
	float GetDyingScore()const { return dying_score; }

	//生存スコアを取得
	float GetActiveScore()const { return activ_score; }

	//対象のHPスコア
	float GetTargetHpScore()const { return target_hp_score; }

private:
	//平均HPの割合を算出
	void AverageHp(const std::vector<GameObject*>& game_objects);

	//部隊の瀕死人数・生存人数を算出
	void MortalityRate(const std::vector<GameObject*>& game_objects);

private:
	float average_hp_score;		//平均HPのスコア
	float dying_score;			//瀕死スコア
	float activ_score;			//生存スコア
	float target_hp_score;		//対象のHPスコア
};

