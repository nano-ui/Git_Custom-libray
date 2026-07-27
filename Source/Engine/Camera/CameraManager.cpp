#include "CameraManager.h"
#include "Camera.h"

#include <Windows.h>

//シングルトン取得
CameraManager& CameraManager::Instance()
{
	static CameraManager instance;
	return instance;
}

//アクティブカメラの設定
void CameraManager::SetActiveCamera(const std::shared_ptr<Camera>& camera)
{
	active_camera = camera;
}

//アクティブカメラを取得
std::shared_ptr<Camera> CameraManager::GetActiveCamera() const
{
	return active_camera.lock();
}

//入力値からカメラ基準の水平移動ベクトルを計算
DirectX::XMFLOAT3 CameraManager::CalculateMoveVector(float input_x, float input_z) const
{
	std::shared_ptr<Camera> camera = active_camera.lock();
	if (!camera)
	{
		OutputDebugStringA("[CameraManager 警告] CalculateMoveVector: アクティブカメラが設定されていないか破棄されています。ワールド軸で計算します。\n");
		return DirectX::XMFLOAT3(input_x, 0.0f, input_z);
	}

	//水平化されたカメラの前方向・右方向ベクトル取得
	DirectX::XMVECTOR front_vec = GetHorizontalFront(camera.get());
	DirectX::XMVECTOR right_vec = GetHorizontalRight(camera.get());

	//移動ベクトル算出
	DirectX::XMVECTOR move_vec = DirectX::XMVectorAdd(
		DirectX::XMVectorScale(front_vec, input_z),
		DirectX::XMVectorScale(right_vec, input_x)
	);

	DirectX::XMFLOAT3 result = {};
	DirectX::XMStoreFloat3(&result, move_vec);
	return result;
}

// 水平方向の前方ベクトル計算
DirectX::XMVECTOR CameraManager::GetHorizontalFront(const Camera* camera) const
{
	DirectX::XMFLOAT3 front = camera->GetFront();
	DirectX::XMVECTOR front_vec = DirectX::XMVectorSet(front.x, 0.0f, front.z, 0.0f);

	//入力チェック
	if (DirectX::XMVector3Equal(front_vec, DirectX::XMVectorZero()))
	{
		OutputDebugStringA("[CameraManager 警告] GetHorizontalFront: 前方ベクトルが垂直を向いています。\n");
		return DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	return DirectX::XMVector3Normalize(front_vec);
}

// 水平方向の右方向ベクトル計算
DirectX::XMVECTOR CameraManager::GetHorizontalRight(const Camera* camera) const
{
	DirectX::XMFLOAT3 right = camera->GetRight();
	DirectX::XMVECTOR right_vec = DirectX::XMVectorSet(right.x, 0.0f, right.z, 0.0f);

	//入力チェック
	if (DirectX::XMVector3Equal(right_vec, DirectX::XMVectorZero()))
	{
		OutputDebugStringA("[CameraManager 警告] GetHorizontalRight: 右方向ベクトルが不整合です。\n");
		return DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	}

	return DirectX::XMVector3Normalize(right_vec);
}

