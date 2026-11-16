#pragma once
#include "Gameplay\Components\Base\Component.h"

#include <memory>

class HateComponent;
class TransformComponent;
class JsonSerializer;
class GuiInspector;

class TargetSelectorComponent : public Component
{
public:
	//コンストラクタ
	TargetSelectorComponent();

	//デストラクタ
	~TargetSelectorComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

	//ヘイトコンポーネントを設定
	void SetHateComponent(std::shared_ptr<HateComponent> hate);

	//ヘイトコンポーネントの取得
	std::shared_ptr<HateComponent> GetHateComponent()const;

	//行列コンポーネントを設定
	void SetTargetTransform(std::shared_ptr<TransformComponent> transform);

	//行列コンポーネントの取得
	std::shared_ptr<TransformComponent> GetTransformComponent()const;

private:
	//ヘイト対象を検索して座標コンポーネントを取得
	std::shared_ptr<TransformComponent> SearchHateTarget(uint32_t search_id);

private:
	std::weak_ptr<HateComponent> hate_component;			//ヘイトコンポーネント
	std::weak_ptr<TransformComponent> target_transform;		//対象の座標
	uint32_t target_id;										//対象のID
};

