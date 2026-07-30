#pragma once

#include <memory>
#include <functional>
#include <DirectXMath.h>

class Camera;
class FollowCameraComponent;

//カメラ制御モード
enum class CameraMode
{
	Free,	//フリーカメラ
	Follow	//追従カメラ
};

class CameraController
{
public:
	//コンストラクタ
	CameraController();

	//デストラクタ
	~CameraController();

	//初期化処理
	void Initialize();

	//更新処理
	void Update(float elapsed_time);

	//ImGui描画
	void RenderGui();

	//追従対象の座標設定
	void SetTargetPositionGetter(const std::function<bool(DirectX::XMFLOAT3&)>& getter);

	//メインカメラを取得
	std::shared_ptr<Camera> GetCamera()const { return main_camera; }

private:
	//モード切替
	void SwitchMode();

private:
	static constexpr int KEY_SWITCH_MODE = 'C'; // モード切り替えキー定数
	std::shared_ptr<Camera>	main_camera;							//アクティブカメラ
	std::unique_ptr<FollowCameraComponent> follow_camera_component;	//追従カメラ
	CameraMode current_mode = CameraMode::Free;						//カメラの状態
};

