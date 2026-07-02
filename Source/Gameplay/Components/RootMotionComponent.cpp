#include "RootMotionComponent.h"

//コンストラクタ
RootMotionComponent::RootMotionComponent()
{
	root_motion = std::make_unique<GltfRootMotion>();
}

//デストラクタ
RootMotionComponent::~RootMotionComponent()
{

}

//初期化
void RootMotionComponent::Initialize(const std::shared_ptr<const GltfModelData>& data, int root_node_index)
{
	root_motion->Initialize(data, root_node_index);
}

//アニメーション切り替え時のリセット
void RootMotionComponent::OnAnimationChaanged(size_t new_animation_index)
{
	current_anim_index = new_animation_index;
	root_motion->ResetDelta();
}

//ルートモーション更新
void RootMotionComponent::Update(float current_animation_time)
{
	//有効フラグまたは計算クラスが無効の場合は更新しない
	if (!is_enabled || !root_motion)
	{
		return;
	}

	root_motion->Update(current_anim_index, current_animation_time);
}

//位置の差分を取得
DirectX::XMFLOAT3 RootMotionComponent::GetDeltaPosition() const
{
	// 無効な場合は差分ゼロを返す
	if (!is_enabled)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return root_motion->GetDeltaPosition();
}

//回転の差分を取得
DirectX::XMFLOAT4 RootMotionComponent::GetDeltaRotation() const
{
	// 無効な場合は回転なしを返す
	if (!is_enabled)
	{
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}

	return root_motion->GetDeltaRotation();
}