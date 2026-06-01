#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

//拡張子に応じて適切なモデルを管理する統合クラス
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

	//頂点座標リストの取得
	std::vector<DirectX::XMFLOAT3> GetVertices()const;

	//インデックスリストの取得
	std::vector<uint32_t> GetIndices()const;

public:
	class ModelImpl;						//実際の処理を行う内部クラス
private:
	std::unique_ptr<ModelImpl> model_impl;	//モデルの実体を保持
};