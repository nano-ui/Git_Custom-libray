#pragma once

#include "Gameplay\Components\Base\Component.h"
#include "Engine\Collision\Collider.h"

#include <memory>
#include <string>
#include <DirectXMath.h>

class ModelComponent;
class ShapeRenderer;

class BoneCapsuleColliderComponent :public Component
{
public:
	//コンストラクタ
	BoneCapsuleColliderComponent();

	//デストラクタ
	~BoneCapsuleColliderComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer);

	//GUIインスペクター登録
	void SetupInspector(GuiInspector* inspector)override;

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//追従対象のモデルコンポーネント登録
	void SetModelComponent(const std::shared_ptr<ModelComponent>& model_comp);

	//単一ボーン設定
	void SetAttachBone(const std::string& bone_name);

	//2ボーン連携設定
	void SetAttachBones(const std::string& start_name, const std::string end_name);

	//パラメータ設定
	void SetRadius(float r) { radius = r; }
	void SetHeight(float h) { height = h; }
	void SetOffset(const DirectX::XMFLOAT3& off) { offset = off; }
	void SetAttribute(ColliderAttribute attr);
	void SetListener(ICollisionListener* listener);

	//コライダー取得
	CapsuleCollider* GetCapsuleCollider() { return capsule_collider.get(); }
	Collider* GetRawCollider() { return capsule_collider.get(); }

private:
	//ボーン追従座標更新処理
	void UpdateTransform();

private:
	std::unique_ptr<CapsuleCollider> capsule_collider;	//カプセルコライダー実体
	std::weak_ptr<ModelComponent> target_model;			//追従対象のモデル
	std::string start_bone_name = "";					//基準(始点)ボーン名
	std::string end_bone_name = "";						//終点ボーン名
	float radius = 0.3f;								//カプセルの半径
	float height = 0.5f;								//カプセルの高さ
	DirectX::XMFLOAT3 offset = { 0.0f,0.0f,0.0f };		//基準ボーンからのローカルオフセット
};

