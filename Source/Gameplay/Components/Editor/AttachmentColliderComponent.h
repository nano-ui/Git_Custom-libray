#pragma once

#include "Gameplay\Components\Base\Component.h"
#include "Engine\Collision\Collider.h"
#include "Editor\Attachment\ColliderAttachmentEditor.h"

#include <vector>
#include <memory>
#include <string>

class ModelComponent;
class TransformComponent;
class ShapeRenderer;

class AttachmentColliderComponent : public Component
{
public:
	//コンストラクタ
	AttachmentColliderComponent();

	//デストラクタ
	~AttachmentColliderComponent();

	//初期化処理
	void Initializa(
		const std::string& model_file_path,
		std::shared_ptr<ModelComponent> model_comp,
		std::shared_ptr<TransformComponent> transform_comp
	);

	//更新処理
	void Update(float elapsed_time)override;

	//デバッグ描画処理
	void RenderDebug(ShapeRenderer* renderer);

	//コライダー名を指定して有効/無効切り替え
	void SetColliderActive(const std::string& target_name, bool is_active);

	//攻撃判定属性のコライダーのみ一括で有効/無効切り替え
	void SetAttackCollidersActive(bool is_active);

	//全コライダーの一括有効/無効切り替え
	void SetAllColluderActive(bool is_active);

	//ランタイムコライダーリスト取得
	const std::vector<std::unique_ptr<CapsuleCollider>>& GetRuntimeCollider()const { return runtime_colliders; }

private:
	//JSONファイルからコライダー設定復元
	bool LoadFromJson(const std::string& file_path);

	//カプセル座標の計算
	bool CalculateCapsulePoints(
		const ColliderAttachmentItem& item,
		ModelComponent* model_comp,
		DirectX::XMFLOAT3& out_start,
		DirectX::XMFLOAT3& out_end
	);

private:
	std::string model_name = "";	//モデル名
	std::vector<std::unique_ptr<ColliderAttachmentItem>> collider_items;	//コライダー定義リスト
	std::vector<std::unique_ptr<CapsuleCollider>> runtime_colliders;		//判定用コライダー

	std::weak_ptr<ModelComponent> model_component;							//参照モデルコンポーネント
	std::weak_ptr<TransformComponent> transform_component;					//参照トランスフォームコンポーネント
};

