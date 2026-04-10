#include "GltfBone.h"

//=================================================================
//コンストラクタ
//=================================================================	
GltfBone::GltfBone()
{
	parent_index = -1;	//親ボーンなし

	//-----------------------------------------------------------------
	//行列の初期化
	//-----------------------------------------------------------------
	DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();	//DirectXMathを使用して単位行列を作成

	DirectX::XMStoreFloat4x4(&local_matrix, identity);	//ローカル行列を単位行列で初期化
	DirectX::XMStoreFloat4x4(&global_matrix, identity);	//グローバル行列を単位行列で初期化
	DirectX::XMStoreFloat4x4(&offset_matrix, identity);	//オフセット行列を単位行列で初期化
}
