#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "../GltfModel/GltfModelData.h"

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

	//モデル情報取得
	std::shared_ptr<GltfModelData> GetData()const { return data; }

	//アニメーション情報取得
	const std::vector<GltfModelData::node>& GetAnimatedNodes() const;

private:
	std::shared_ptr<GltfModelData> data;			//リソースデータ
	std::shared_ptr<GltfModelRenderer> renderer;	//描画命令クラス
	std::unique_ptr<GltfModelAnimation> animation;	//アニメーション計算クラス
};

