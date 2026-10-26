#pragma once

#include "Gameplay\Components\Base\Component.h"

#include <memory>
#include <DirectXMath.h>

class JsonSerializer;
class GuiInspector;
class TransformComponent;
class ShapeRenderer;

class VisionSensorComponent : public Component
{
public:
	static constexpr float DEFAULT_VIEW_DISTANCE = 15.0f;			//視界の最大距離の既定値
	static constexpr float DEFAULT_VIEW_ANGLE = DirectX::XM_PIDIV2;	//視野角の既定値	

public:
	//コンストラクタ
	VisionSensorComponent();

	//仮想デストラクタ
	~VisionSensorComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(ID3D11DeviceContext* context)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer) override;

	//Gui描画
	void RenderGui()override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

	//持ち主の設定
	void SetTransformComponent(const std::shared_ptr<TransformComponent>& transform) { target_transform = transform; }

	//視界内に入ったか判定
	bool IsTargetInSight(const DirectX::XMFLOAT3& target_position);

	//視界の最大距離を取得
	float GetViewDistance()const { return view_distance; }

	//視界の最大距離の設定
	void SetViewDistance(float distance) { view_distance = distance; }

	//視野角の取得
	float GetViewAngle()const { return view_angle; }

	//視野角の設定
	void SetViewAngle(float angle) { view_angle = angle; }

	//発見フラグを取得
	bool IsSight()const { return is_sight; }

private:
	std::weak_ptr<TransformComponent> target_transform;	//視線の持ち主
	float view_distance;								//視界の最大距離
	float view_angle;									//扇形の視野角
	bool is_sight;										//発見フラグ
};

