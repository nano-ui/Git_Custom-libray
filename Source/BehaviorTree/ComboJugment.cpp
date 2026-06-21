#include "ComboJugment.h"

#include <random>

//======================
//コンストラクタ
//======================
ComboJugment::ComboJugment(int min_combo, int max_combo)
	:min_combo_count(min_combo),
	max_combo_count(max_combo),
	target_combo_count(0),
	current_combo_count(0)
{
	UpdateTargetLimit();
}

//===========
//判定処理
//===========
bool ComboJugment::Check()
{
	//========================
	//カウントアップと判定
	//========================
	current_combo_count++;	//コンボ数を進める
	if (current_combo_count <= target_combo_count)	//上限に達しているか判定
	{
		return true;	//攻撃を許可
	}

	//=======================
	//コンボ終了とリセット
	//=======================
	current_combo_count = 0;	//コンボ数を初期化
	UpdateTargetLimit();		//次回のコンボ上限を決定

	return false;	//攻撃停止
}

//===================
//目標回数の更新
//===================
void ComboJugment::UpdateTargetLimit()
{
	//---------------
	//乱数を生成
	//---------------
	std::random_device seed_gen;	//シード値
	std::mt19937 engin(seed_gen());	//エンジンを初期化
	std::uniform_int_distribution<int> dist(min_combo_count, max_combo_count);	//範囲を定義
	target_combo_count = dist(engin);	//目標コンボ数を設定
}
