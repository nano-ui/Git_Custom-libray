#pragma once
#include "JudgmentNode.h"
class ComboJugment :  public JudgmentNode
{
public:
	ComboJugment(int min_combo, int max_combo);

	//判定処理
	virtual bool Check()override;

	//現在のコンボ数を取得
	int GetCurrentComboCount()const { return current_combo_count; }

	//目標コンボ数を取得
	int GetTargetComboCount()const { return target_combo_count; }

private:
	//目標回数の更新
	void UpdateTargetLimit();

	int min_combo_count;		//攻撃の最小回数
	int max_combo_count;		//攻撃の最大回数
	int target_combo_count;		//目標のコンボ数
	int current_combo_count;	//現在のコンボ数
};

