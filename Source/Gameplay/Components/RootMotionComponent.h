#pragma once

#include "Engine\Graphics\GltfModel\GltfRootMotion.h"

#include <memory>
#include <DirectXMath.h>

class RootMotionComponent
{
public:
	//コンストラクタ
	RootMotionComponent();

	//デストラクタ
	~RootMotionComponent();

	//初期化
	void Initialize(const std::shared_ptr<const GltfModelData>& data, int root_node_index = 0);

	//アニメーション切り替え時のリセット
	void OnAnimationChaanged(size_t new_animation_index);

	//ルートモーション更新
	void Update(float current_animation_time);

	//ルートモーションのフラグ設定
	void SetEnable(bool enable) { is_enabled = enable; }

	//位置の差分を取得
	DirectX::XMFLOAT3 GetDeltaPosition()const;

	//回転の差分を取得
	DirectX::XMFLOAT4 GetDeltaRotation()const;

private:
	std::unique_ptr<GltfRootMotion> root_motion;	//ルートモーション計算クラス
	size_t current_anim_index = 0;					//再生中のアニメーション番号
	bool is_enabled = true;							//ルートモーション再生フラグ
};

