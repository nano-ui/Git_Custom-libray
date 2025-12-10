#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "AssetManager.h"

struct Transform
{
	DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };//クォータニオン用
	DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f };
};

class Model
{
public:
	//AssetManagerにロードを依頼し、その結果を元にインスタンスを初期化
	Model(AssetManager* asset_manager, const wchar_t* fbx_filename);


private:
	bool is_skinned_;//AssetManager::LoadModelAssetの結果を保持
	bool mesh_index_;//AssetManager内の対応するメッシュのインデックス
};

