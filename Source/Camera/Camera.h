#pragma once

#include <DirectXMath.h>

// カメラ
class Camera
{
public:
	//コンストラクタ
	Camera();

	//デストラクタ
	virtual ~Camera();

	//初期化
	virtual void Initialize() = 0;

	//更新処理
	virtual void Update(float elapsed_time) = 0;

	//ImGui描画処理
	virtual void RenderGui() = 0;

	// 指定方向を向く
	void SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up);

	// パースペクティブ設定
	void SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

	// ビュー行列取得
	const DirectX::XMFLOAT4X4& GetView() const { return view; }

	// プロジェクション行列取得
	const DirectX::XMFLOAT4X4& GetProjection() const { return projection; }

	// 視点取得
	const DirectX::XMFLOAT3& GetEye() const { return eye; }

	// 注視点取得
	const DirectX::XMFLOAT3& GetFocus() const { return focus; }

	// 上方向取得
	const DirectX::XMFLOAT3& GetUp() const { return up; }

	// 前方向取得
	const DirectX::XMFLOAT3& GetFront() const { return front; }

	// 右方向取得
	const DirectX::XMFLOAT3& GetRight() const { return right; }

protected:
	DirectX::XMFLOAT4X4		view;		//ビュー変換行列
	DirectX::XMFLOAT4X4		projection;	//プロジェクション変換行列

	DirectX::XMFLOAT3		eye;		//視点座標
	DirectX::XMFLOAT3		focus;		//注視点座標

	DirectX::XMFLOAT3		up;			//上方向ベクトル
	DirectX::XMFLOAT3		front;		//前方向ベクトル
	DirectX::XMFLOAT3		right;		//右方向ベクトル
};
