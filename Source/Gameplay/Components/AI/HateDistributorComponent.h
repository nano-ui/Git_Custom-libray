#pragma once
#include "Gameplay\Components\Base\Component.h"

class JsonSerializer;
class GuiInspector;

class HateDistributorComponent : public Component
{
public:
	//コンストラクタ
	HateDistributorComponent();

	//デストラクタ
	~HateDistributorComponent()override;

	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

private:
	//ヘイト値を配る
	void DistributeInitialHate();

private:
	float initial_hate;	//初期化時のヘイト値
	bool is_distributed;//初期ヘイトフラグ
};

