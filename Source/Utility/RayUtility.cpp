#include "RayUtility.h"
#include "Windows.h"

//クリック座標を算出
bool ScreenToWorldRay(
	float window_height,
	float window_width,
	DirectX::XMFLOAT2 click_pos,
	DirectX::XMFLOAT4X4 view,
	DirectX::XMFLOAT4X4 projection,
	DirectX::XMFLOAT3& out_ray_start,
	DirectX::XMFLOAT3& out_ray_end)
{
	//スクリーン座標から正規化デバイス座標（NDC）への変換
	DirectX::XMFLOAT2 ndc = { 0.0f,0.0f };
	DirectX::XMFLOAT2 ratio = { 0.0f,0.0f };

	ratio.x = click_pos.x / window_width;	//画面の比率（0.0 ～ 1.0）への正規化
	ratio.x *= 2.0f;						//範囲を2倍に拡張（0.0 ～ 2.0）
	ndc.x = ratio.x - 1.0f;					//中央を原点にシフト（-1.0 ～ +1.0）

	ratio.y = click_pos.y / window_height;
	ratio.y *= 2.0f;
	ndc.y = 1.0f - ratio.y;

	//ビュープロジェクション行列の逆行列化
	DirectX::XMMATRIX matrix;
	DirectX::XMMATRIX m_view = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX m_projectoin = DirectX::XMLoadFloat4x4(&projection);
	matrix = DirectX::XMMatrixMultiply(m_view, m_projectoin);
	matrix = DirectX::XMMatrixInverse(nullptr, matrix);

	if (DirectX::XMMatrixIsInfinite(matrix))
	{
		OutputDebugStringA("[RayUtility エラー] ScreenToWorldRay: ビュープロジェクション行列の逆行列計算に失敗しました（行列が無限大です）。\n");
		return false;
	}

	//NDC座標からワールド空間レイの算出
	DirectX::XMFLOAT3 near_pos = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 far_pos = { 0.0f,0.0f,0.0f };
	DirectX::XMVECTOR v_ndc_near = DirectX::XMVectorSet(ndc.x, ndc.y, 0.0f, 1.0f);
	DirectX::XMVECTOR v_ndc_far = DirectX::XMVectorSet(ndc.x, ndc.y, 1.0f, 1.0f);
	DirectX::XMVECTOR v_warld_near = DirectX::XMVector3TransformCoord(v_ndc_near, matrix);
	DirectX::XMVECTOR v_warld_far = DirectX::XMVector3TransformCoord(v_ndc_far, matrix);
	DirectX::XMVECTOR v_diff = DirectX::XMVectorSubtract(v_warld_far, v_warld_near);
	v_diff = DirectX::XMVector3Normalize(v_diff);
	
	//変換処理
	DirectX::XMFLOAT3 direction = { 0.0f,0.0f,0.0f };
	DirectX::XMStoreFloat3(&direction, v_diff);
	DirectX::XMStoreFloat3(&out_ray_start, v_warld_near);
	DirectX::XMStoreFloat3(&out_ray_end, v_warld_far);

	return true;
}
