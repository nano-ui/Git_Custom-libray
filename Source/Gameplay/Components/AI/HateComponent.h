#pragma once
#include "Gameplay\Components\Base\Component.h"

#include <unordered_map>

//ヘイト管理
class HateComponent : public Component
{
public:
	//コンストラクタ
	HateComponent();

	//デストラクタ
	~HateComponent()override;

	//初期化処理
	void Initialize();

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(ID3D11DeviceContext* context) {}

	//デバッグ描画処理
	void RenderGui()override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

	//ヘイト加算
	void AddHate(uint32_t target_id, float hate);

	//最大ヘイト値ID取得
	uint32_t GetTopHateTargetId();

	//ヘイト対象が存在するか判定
	bool HasTarget()const;

private:
	static constexpr uint32_t NOT_TARGET_ID = 0;		//非注目対象ID
	static constexpr float DEFAULT_DECAY_RATE = 1.0f;	//初期ヘイト値
	std::unordered_map<uint32_t, float> hate_table;		//ヘイトコンテナ
	float decay_rate;									//ヘイト値減衰量

	uint32_t cached_top_hate_id = NOT_TARGET_ID;		//最大ヘイトを持つ対象のID
	uint32_t debug_top_hate_id;							//デバッグ用最大ヘイトID
	float debug_top_hate_value;							//デバッグ用最大ヘイト値
};

