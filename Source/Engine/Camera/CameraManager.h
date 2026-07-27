#pragma once

#include <DirectXMath.h>
#include <memory>

class Camera;

class CameraManager
{
public:
	//シングルトン取得
	static CameraManager& Instance();

	//アクティブカメラの設定
	void SetActiveCamera(const std::shared_ptr<Camera>& camera);

	//アクティブカメラを取得
	std::shared_ptr<Camera> GetActiveCamera()const;

	//入力値からカメラ基準の水平移動ベクトルを計算
	DirectX::XMFLOAT3 CalculateMoveVector(float input_x, float input_z)const;

private:
	// コンストラクタ / デストラクタ
	CameraManager() = default;
	~CameraManager() = default;

	// コピー・代入禁止
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	// 水平方向の前方ベクトル計算
	DirectX::XMVECTOR GetHorizontalFront(const Camera* camera) const;

	// 水平方向の右方向ベクトル計算
	DirectX::XMVECTOR GetHorizontalRight(const Camera* camera) const;

private:
	std::weak_ptr<Camera> active_camera;	//現在のメインカメラ
};

