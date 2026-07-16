#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "../Engine/Graphics/GltfModel/GltfModelData.h"

class GltfModelAnimation;
class GltfModelRenderer;
struct ID3D11DeviceContext;

class GltfModel
{
public:
	//コンストラクタ
	GltfModel(const std::shared_ptr<GltfModelData>& model_data, const std::shared_ptr<GltfModelRenderer> model_renderer);

	//デストラクタ
	~GltfModel();

	//更新処理
	void Update(float delta_time);

	//描画処理
	void Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world_matrix);

	//アニメーション切り替え
	void PlayAnimation(const std::string& animation_name, bool is_loop);

	//アニメーションの終了したか取得
	bool IsAnimationFinished()const;

	//モデル情報取得
	std::shared_ptr<GltfModelData> GetData()const { return data; }

	//アニメーション情報取得
	const std::vector<GltfModelData::node>& GetAnimatedNodes() const;

	//現在の再生経過時間を取得
	float GetAnimationCurrentTime() const;

	//再生時間を設定
	void SetAnimationTime(float time);

	//アニメーションの総時間を取得
	float GetAnimationDuration() const;

	//指定したノードの移動成分を外部から上書き
	void SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation);

	//変更されたノード情報をもとに、グローバル行列を再計算
	void RecalculateTransforms();

private:
	std::shared_ptr<GltfModelData> data;			//リソースデータ
	std::shared_ptr<GltfModelRenderer> renderer;	//描画命令クラス
	std::unique_ptr<GltfModelAnimation> animation;	//アニメーション計算クラス
};

