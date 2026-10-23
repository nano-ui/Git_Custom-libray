#include "RootMotionComponent.h"

#include <cstdio>
#include <Engine\Core\MacroTracy.h>

//コンストラクタ
RootMotionComponent::RootMotionComponent()
{
	BS_PROFILE_FUNCTION();

	root_motion = std::make_unique<GltfRootMotion>();
}

//デストラクタ
RootMotionComponent::~RootMotionComponent()
{

}


//初期化
void RootMotionComponent::Initialize(const std::shared_ptr<const GltfModelData>& data)
{
	BS_PROFILE_FUNCTION();

	root_motion->Initialize(data);
}

//アニメーション切り替え時のリセット
void RootMotionComponent::OnAnimationChaanged(size_t new_animation_index)
{
	BS_PROFILE_FUNCTION();

	current_anim_index = new_animation_index;
	root_motion->ResetDelta();
}

//ルートモーション更新
void RootMotionComponent::Update(float current_animation_time)
{
	BS_PROFILE_FUNCTION();

	//有効フラグまたは計算クラスが無効の場合は更新しない
	if (!is_enabled || !root_motion)
	{
		return;
	}
	if (is_updated_this_frame)
	{
		//printf("[RootMotionComponent 警告] Update: 1フレーム内に複数回の呼び出しを検知したため、重複更新をガードしました。\n");
		return;
	}
	root_motion->Update(current_anim_index, current_animation_time);
	is_updated_this_frame = true;
}

//位置の差分を取得
DirectX::XMFLOAT3 RootMotionComponent::GetDeltaPosition() const
{
	BS_PROFILE_FUNCTION();

	// 無効な場合は差分ゼロを返す
	if (!is_enabled)
	{
		return { 0.0f, 0.0f, 0.0f };
	}
	is_updated_this_frame = false;
	return root_motion->GetDeltaPosition();
}

//回転の差分を取得
DirectX::XMFLOAT4 RootMotionComponent::GetDeltaRotation() const
{
	BS_PROFILE_FUNCTION();

	// 無効な場合は回転なしを返す
	if (!is_enabled)
	{
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}

	return root_motion->GetDeltaRotation();
}

//ルートノードのインデックスを取得
int RootMotionComponent::GetTargetNodeIndex() const
{
	BS_PROFILE_FUNCTION();

	if (!root_motion)return 0;
	return root_motion->GetTargetNodeIndex();
}

//ルートモーションの抽出値を検証
void RootMotionComponent::TraceRootMotionDebug(const DirectX::XMFLOAT3& raw_position)
{
	BS_PROFILE_FUNCTION();

	// 出力バッファの確保
	char debugStr[256];

	// GetDeltaPosition() の結果を一時に格納
	DirectX::XMFLOAT3 delta = GetDeltaPosition();

	//printf("[RootMotion Debug] Raw: %.4f, %.4f, %.4f | Delta: %.4f, %.4f, %.4f\n", raw_position.x, raw_position.y, raw_position.z, delta.x, delta.y, delta.z);
}
