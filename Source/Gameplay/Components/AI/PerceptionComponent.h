#pragma once
#include "Gameplay\Components\Base\Component.h"

#include <memory>

class VisionSensorComponent;
class HateComponent;
enum class GameObjectTag : uint32_t;

//知覚コンポーネントクラス
class PerceptionComponent : public Component
{
public:
	//コンストラクタ
	PerceptionComponent();

	//デストラクタ
	~PerceptionComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time) override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

public:
	//タグの設定
	void SetTargetTag(GameObjectTag tag) { target_tag = tag; }

	//タグの取得
	GameObjectTag GetTargetTag()const { return target_tag; }

	//ヘイト増加量の設定
	void SetHatePerSecond(float rate) { hate_per_second = rate; }

	//ヘイト増加量の取得
	float GetHatePerSecond()const { return hate_per_second; }

	//視界の持ち主の設定
	void SetVisionSensor(const std::shared_ptr<VisionSensorComponent>& sensor) noexcept { vision_sensor = sensor; }

	//ヘイトの持ち主の設定
	void SetHate(const std::shared_ptr<HateComponent>& my_hate)noexcept { hate = my_hate; }

private:
	GameObjectTag target_tag;							//検知対象のタグ
	float hate_per_second;								//ヘイト増加量
	std::weak_ptr<VisionSensorComponent> vision_sensor;	//持ち主の視界
	std::weak_ptr<HateComponent> hate;					//持ち主のヘイト
};

