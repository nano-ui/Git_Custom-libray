#pragma once

#include "Gameplay\Components\Base\Component.h"
#include "Engine\Collision\Collider.h"

#include <memory>
#include <DirectXMath.h>

class TransformComponent;
class JsonSerializer;
class GuiInspector;

class ColliderComponent :public Component
{
public:
	//コンストラクタ
	ColliderComponent();

	//デストラクタ
	virtual ~ColliderComponent();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//inspector登録
	void SetupInspector(GuiInspector* inspector)override;

	//トランスフォームの設定
	void SetTransformComponent(const std::shared_ptr<TransformComponent>& transform);

	//実体コライダーのポインタ取得
	virtual Collider* GetRawCollider() = 0;

	//衝突イベントの設定
	void SetListener(ICollisionListener* listener);

	//属性の設定
	void SetAttribute(ColliderAttribute attr);

	//属性の取得
	ColliderAttribute GetAttribute()const { return attribute; }

	//重量の設定
	void SetWeight(float w);

	//重量の取得
	float GetWeight()const { return weight; }

	//偏り座標の設定
	void SetOffset(const DirectX::XMFLOAT3& off);

	//偏り座標の取得
	const DirectX::XMFLOAT3& GetOffset()const { return offset; }

protected:
	//形状ごとの同期処理
	virtual void UpdateColliderTransform(const DirectX::XMFLOAT3& world_pos, const DirectX::XMFLOAT4& world_rot) = 0;

	//CollisionManagerへの登録
	void RegisterToManager();

	//CollisionManagerへの解除
	void UnregisterFromManager();

protected:
	std::weak_ptr<TransformComponent> target_transform;		//対象のTransform
	DirectX::XMFLOAT3 offset;								//相対偏り座標
	float weight;											//当たり判定の重さ
	ColliderAttribute attribute;							//当たり判定の属性
	bool is_registered = false;								//登録フラグ
};

