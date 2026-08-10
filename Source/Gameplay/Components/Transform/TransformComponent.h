#pragma once

#include "Gameplay\Components\Base\Component.h"

#include <DirectXMath.h>

class TransformComponent : public Component
{
public:
	//コンストラクタ
	TransformComponent();

	//仮想デストラクタ
	virtual ~TransformComponent();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//ImGuiデバッグ描画処理
	void RenderGui()override;

	//座標の取得
	const DirectX::XMFLOAT3& GetPosition()const { return position; }

	//座標の設定
	void SetPosition(const DirectX::XMFLOAT3& pos);

	//オイラー角の取得
	const DirectX::XMFLOAT3 GetRotation()const { return rotation; }

	//オイラー角の設定
	void SetRotation(const DirectX::XMFLOAT3& rot);

	//クォータニオンの取得
	const DirectX::XMFLOAT4& GetQuaternion()const { return quaternion; }

	//クォータニオンの設定
	void SetRotationQuaternion(const DirectX::XMFLOAT4& rot);

	//スケールの取得
	const DirectX::XMFLOAT3 GetScale()const { return scale; }

	//スケールの設定
	void SetScale(const DirectX::XMFLOAT3& scl);

	//ワールド座標の取得
	const DirectX::XMMATRIX& GetWorldMatrix()const { return world_matrix; }

private:
	//行列とクォータニオンの計算処理
	void UpdateWorldMatrix();

private:
	DirectX::XMFLOAT3 position;		//位置座標
	DirectX::XMFLOAT3 rotation;		//オイラー角
	DirectX::XMFLOAT4 quaternion;	//クォータニオン
	DirectX::XMFLOAT3 scale;		//拡大率
	DirectX::XMMATRIX world_matrix;	//ワールド座標
	bool is_dirty;					//行列計算フラグ
};

