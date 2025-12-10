#include "Model.h"
#include <DirectXMath.h>

Model::Model(AssetManager* asset_manager, const wchar_t* fbx_filename)
{
	//AssetManagerにFBXファイルのロードを依頼し、結果（インデックスとタイプ）を受け取る
	MeshReference ref = asset_manager->LoadModelAsset(fbx_filename);

	is_skinned_ = ref.is_skinned;
	mesh_index_ = ref.mesh_index;
}

//Transformからワールド行列を計算
DirectX::XMFLOAT4X4 Model::GetWorldMatrix() const
{
	//スケーリング行列を生成
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(transform_.scale.x, transform_.scale.y, transform_.scale.z);

	//回転行列を生成(クォータニオンを使用)
	DirectX::XMVECTOR Q = DirectX::XMLoadFloat4(&transform_.rotation);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(Q);

	//移動行列を生成
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(transform_.position.x, transform_.position.y, transform_.position.z);

	//ワールド行列を計算: S * R * T
	DirectX::XMMATRIX world_matrix = S * R * T;

	//結果をXMFLOAT4X4に格納して返す
	DirectX::XMFLOAT4X4 result;
	DirectX::XMStoreFloat4x4(&result, world_matrix);

	return result;
}
