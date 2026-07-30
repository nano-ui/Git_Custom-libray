#include "CameraController.h"
#include "Camera.h"
#include "FreeCamera.h"
#include "FollowCameraComponent.h"
#include "CameraManager.h"
#include "Engine\Core\Input.h"

#include <Windows.h>

//コンストラクタ
CameraController::CameraController()
{
}

//デストラクタ
CameraController::~CameraController()
{
}

//初期化処理
void CameraController::Initialize()
{
	main_camera = std::make_shared<FreeCamera>();
	if (main_camera)main_camera->Initialize();
	else OutputDebugStringA("[CameraController エラー] main_camera の生成に失敗しました。\n");

	follow_camera_component = std::make_unique<FollowCameraComponent>();
	if (follow_camera_component)follow_camera_component->SetCamera(main_camera);
	else OutputDebugStringA("[CameraController エラー] follow_camera_component の生成に失敗しました。\n");

	CameraManager::Instance().SetActiveCamera(main_camera);
}

//更新処理
void CameraController::Update(float elapsed_time)
{
	if (Input::Instance().IsKeyTrigger(KEY_SWITCH_MODE))SwitchMode();
	if (current_mode == CameraMode::Free)
	{
		if (main_camera)main_camera->Update(elapsed_time);
	}
	else if (current_mode == CameraMode::Follow)
	{
		if (follow_camera_component)follow_camera_component->Update(elapsed_time);
		else OutputDebugStringA("[CameraController エラー] follow_camera_component が nullptr です。\n");
	}
}

//ImGui描画
void CameraController::RenderGui()
{
#ifdef USE_IMGUI
	if (main_camera)main_camera->RenderGui();
	if (current_mode == CameraMode::Follow && follow_camera_component)follow_camera_component->RenderGui();
#endif
}

//追従対象の座標設定
void CameraController::SetTargetPositionGetter(const std::function<bool(DirectX::XMFLOAT3&)>& getter)
{
	follow_camera_component->SetTarget(getter);
}

//モード切替
void CameraController::SwitchMode()
{
	if (current_mode == CameraMode::Free)
	{
		current_mode = CameraMode::Follow;
		follow_camera_component->ResetCameraPosition();
		OutputDebugStringA("[CameraController] カメラモードを「追従モード(Follow)」に切り替えました。\n");
	}
	else
	{
		current_mode = CameraMode::Free;
		OutputDebugStringA("[CameraController] カメラモードを「フリーモード(Free)」に切り替えました。\n");
	}
}
