#pragma once

#include <DirectXMath.h>
#include <memory>

class Camera;
class GameObject;

class FollowCameraComponent
{
public:
	//コンストラクタ
	FollowCameraComponent();

	//デストラクタ
	~FollowCameraComponent();

	//初期化処理
	void Initialize();

	//更新処理
	void Update(float elapsed_time);

	//ImGui描画
	void RenderGui();

	//対象のGameObject参照の設定
	void SetTarget(const std::shared_ptr<const GameObject>& target_obj);

	//制御対象のカメラを設定
	void SetCamera(const std::shared_ptr<Camera>& camera);

private:
	//ターゲットの位置とオフセットから理想のカメラ位置を計算
	DirectX::XMFLOAT3 CalculateTargetEyePosition(const DirectX::XMFLOAT3& target_pos) const;

	//フレームレート非依存の補間計算
	DirectX::XMFLOAT3 InterpolatePosition(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, float speed, float elapsed_time) const;

private:
	static constexpr float DEFAULT_OFFSET_X = 0.0f;
	static constexpr float DEFAULT_OFFSET_Y = 5.0f;
	static constexpr float DEFAULT_OFFSET_Z = -10.0f;
	static constexpr float DEFAULT_FOLLOW_SPEED = 5.0f;

	std::weak_ptr<const GameObject> target_object;				//追従対象オブジェクト
	std::weak_ptr<Camera> target_camera;						//制御対象カメラ
	DirectX::XMFLOAT3						offset_position;	//対象からの相対オフセット位置
	DirectX::XMFLOAT3						look_at_offset;		//対象からの注視点オフセット
	float									follow_speed;		//追従の補間速度
};

