#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Graphics/GltfModel/GltfModelData.h"

class GltfModel;
class GltfModelRenderer;

class Model
{
public:
	// コンストラクタ
	Model(ID3D11Device* device, const std::string& file_path);

	// デストラクタ
	~Model();	

	// 更新処理
	void Update(float elapsed_time);	

	// 描画処理
	void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f });

	// アニメーションの再生
	void PlayAnimation(const std::string& animation_name, bool is_loop);

	// 登録されているすべてのアニメーション名の一覧を取得	
	std::vector<std::string> GetAnimationNames() const;

	// 頂点座標リストの取得	
	std::vector<DirectX::XMFLOAT3> GetVertices() const;

	// インデックスリストの取得	
	std::vector<uint32_t> GetIndices() const;

	// アニメーションが終了したか取得	
	bool IsAnimationFinished() const;

	// モデルパスの取得	
	const std::string& GetModelPath() const { return model_path; }

	// アニメーション再生時間取得	
	float GetAnimationTime() const;

	//アニメーション再生時間を設定
	void SetAnimationTime(float time);

	// アニメーションの総時間を取得	
	float GetAnimationDuration() const;

	// アニメーション名からインデックス番号を取得	
	int GetAnimationIndex(const char* name) const;
	
	// glTFモデルのデータ本体を取得	
	std::shared_ptr<const GltfModelData> GetGltfModelData() const;

	// 初期状態（アニメーション前）のオリジナルノード配列を取得
	std::vector<GltfModelData::node>& GetNodes();

	// アニメーション計算適用後の動的なノード配列を取得
	const std::vector<GltfModelData::node>& GetAnimatedNodes() const;

	// 指定したノードの移動成分を外部から上書きする	
	void SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation);

	// 変更されたノード情報をもとに、グローバル行列を再計算する
	void RecalculateTransforms();

private:
	std::shared_ptr<GltfModelData> data;			// glTFのデータ構造を保持するスマートポインタ
	std::shared_ptr<GltfModelRenderer> renderer;	// 描画を司るレンダラーのスマートポインタ
	std::unique_ptr<GltfModel> model;				// アニメーション更新や制御を行う実体
	std::string model_path = "";					// 読み込んだモデルのファイルパス
};