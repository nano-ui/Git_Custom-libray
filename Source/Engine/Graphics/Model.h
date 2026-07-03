#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

#include "../Engine/Graphics/GltfModel/GltfModelData.h"

class Model
{
public:
	//コンストラクタ
	Model(ID3D11Device* device, const std::string& file_path);

	//デストラクタ
	~Model();

	//更新処理
	void Update(float elapsed_time);

	//描画処理
	void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f });

	//アニメーションの再生
	void PlayAnimation(const std::string& animation_name, bool is_loop);

	//登録されているすべてのアニメーション名の一覧を取得
	std::vector<std::string> GetAnimationNames() const;

	//頂点座標リストの取得
	std::vector<DirectX::XMFLOAT3> GetVertices()const;

	//インデックスリストの取得
	std::vector<uint32_t> GetIndices()const;

	//アニメーションが終了したか取得
	bool IsAnimationFinished() const;

	//モデルパスの取得
	const std::string& GetModelPath()const { return model_path; }

	//アニメーション再生時間取得
	float GetAnimationTime()const;

	//アニメーションの総時間を取得
	float GetAnimationDuration() const;

	//アニメーション名からインデックス番号を取得
	int GetAnimationIndex(const char* name) const;

	//glTFモデルのデータ本体を取得
	std::shared_ptr<const GltfModelData> GetGltfModelData() const;

	//ノードを取得
	std::vector<GltfModelData::node>& GetNodes();

public:
	class ModelImpl;						//実際の処理を行う内部クラス
private:
	std::unique_ptr<ModelImpl> model_impl;	//モデルの実体を保持
	std::string model_path = "";			//読み込んだモデルのパス
};