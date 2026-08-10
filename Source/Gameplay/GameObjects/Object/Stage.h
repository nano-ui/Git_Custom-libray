#pragma once

#include "Gameplay/GameObjects/GameObject.h"
#include "Engine/Collision/Collider.h"

#include <memory>
#include <vector>
#include <DirectXMath.h>

class SpaceDivisionCast;
class TransformComponent;
class ModelComponent;

class Stage : public GameObject
{
public:
	//コンストラクタ
	Stage();

	//デストラクタ
	~Stage();

	//初期化
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(ID3D11DeviceContext* context)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//シリアライザおよび GuiInspector への変数登録
	void SetupSerialization() override;

	//任意のモデル読み込みと空間分割当たり判定の自動再構築
	bool LoadStageModel(const std::string& file_path);

	//当たり判定用の空間分割キャストオブジェクトを取得
	SpaceDivisionCast* GetSpaceDivisionCast();

	//コライダー取得
	SpaceDivisionCollider* GetCollider() { return &space_collider; }
private:
	//空間分割データの構築処理
	void BuildCollisionData();

private:
	std::shared_ptr<TransformComponent> transform_component;
	std::shared_ptr<ModelComponent> model_component;
	std::shared_ptr<const GltfModelData> current_model_data = nullptr; //構築済みモデルデータのキャッシュ
	std::unique_ptr<ShapeRenderer> shape_renderer;			//デバッグ描画クラス
	std::unique_ptr<SpaceDivisionCast> space_division_cast;	//当たり判定空間分割クラス
	bool is_draw_areas = false;
	DirectX::XMFLOAT4 area_draw_color = { 1.0f,0.0f,0.0f,1.0f };
	SpaceDivisionCollider space_collider;
};

