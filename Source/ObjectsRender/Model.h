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

	//position, rotation, scaleからワールド行列を計算
	DirectX::XMFLOAT4X4 GetWorldMatrix()const;

	//モデルがスキニングメッシュを使用しているかを取得
	bool IsSkinned()const { return is_skinned_; }

	//AssetManager内のメッシュ配列のインデックスを取得
	size_t GetMeshIndex()const { return mesh_index_; }

	//モデルのトランスフォームデータを取得
	Transform& GetTransform() { return transform_; }

	// アニメーションの再生時間を取得
	float GetAnimationTime()const { return animation_time_; }

	//アニメーションの再生時間を設定
	void SetAnimationTime(float time) { animation_time_ = time; }

private:
	bool is_skinned_;//AssetManager::LoadModelAssetの結果を保持
	bool mesh_index_;//AssetManager内の対応するメッシュのインデックス
	Transform transform_;//モデルの位置、回転、スケール情報
	float animation_time_;//アニメーションの再生時間(秒)
};

