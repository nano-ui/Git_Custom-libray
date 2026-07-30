#pragma once

#include <DirectXMath.h>
#include <functional>
#include <memory>

class Camera;

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

	//カメラ位置を補間なしで即時ターゲットへワープ
	void ResetCameraPosition() { is_first_frame = true; }

	//追従対象の位置設定
	void SetTarget(const std::function<bool(DirectX::XMFLOAT3&)>& getter);

	//制御対象のカメラを設定
	void SetCamera(const std::shared_ptr<Camera>& camera);

private:
	//マウス入力による回転角の更新
	void UpdateMouseRotation(float elapsed_time);

	//ターゲット座標と角度・距離から理想のカメラ位置を極座標計算
	DirectX::XMFLOAT3 CalculateOrbitEyePosition(const DirectX::XMFLOAT3& focus_pos) const;

	//フレームレート非依存の補間計算
	DirectX::XMFLOAT3 InterpolatePosition(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos, float speed, float elapsed_time) const;

private:
	static constexpr float DEFAULT_OFFSET_Y = 5.0f;			//注視点の基準Yオフセット
	static constexpr float DEFAULT_DISTANCE = 10.0f;		//デフォルトのカメラ距離
	static constexpr float DEFAULT_FOLLOW_SPEED = 10.0f;	//追従の補間速度
	static constexpr float DEFAULT_TURN_SENSITIVITY = 0.2f;	//マウス感度

	static constexpr float MIN_PITCH_DEGREE = -10.0f;		//地面へ潜らないよう下限を制限（-10度）
	static constexpr float MAX_PITCH_DEGREE = 80.0f;		//上限角度（80度）
	static constexpr float INITIAL_PITCH_DEGREE = 20.0f;	//初期状態で見下ろす角度（20度）
	static constexpr float MIN_CAMERA_HEIGHT = 0.5f;		//カメラ最低高度（地面沈み込み防止）

	std::function<bool(DirectX::XMFLOAT3&)>& target_position_getter;	//追従対象の位置座標
	std::weak_ptr<Camera>					target_camera;		//制御対象カメラ
	bool is_first_frame = true;									//初回・切替直後判定フラグ
	DirectX::XMFLOAT3						offset_position;	//対象からの相対オフセット位置
	DirectX::XMFLOAT3						look_at_offset;		//対象からの注視点オフセット
	float									follow_speed;		//追従の補間速度
	DirectX::XMFLOAT2						rotation_angle;		//カメラの回転角（x: Pitch, y: Yaw）
	float									distance;			//対象からの離れ距離
	float									turn_sensitivity;	//マウス回転の感度
};

