#pragma once

#include <DirectXMath.h>
#include <memory>
#include <string>
#include "ThiedParty\json.hpp"

struct ID3D11DeviceContext;
class GltfModel;
class GltfModelData;
class GltfModelRenderer;

class ModelComponent
{
public:
	ModelComponent() = default;
	~ModelComponent() = default;

	//モデルファイルのロードを初期化
	bool Initialize(const std::string& file_path);

	//アニメーション更新
	void Update(float delta_time);

	//描画処理
	void Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world_matrix);

	//ImGui描画
	void DrawImGui();

	//Jsonへのモデルパスデータ保存
	void SaveToObject(nlohmann::json& object_json)const;

	//Jsonへのモデルパスデータ復元とモデルロード
	void LoadFromJObject(const nlohmann::json& object_json);

	//描画表示フラグ設定
	void SetVisible(bool visible) { is_visible = visible; }

	//描画表示フラグ取得
	bool IsVisible()const { return is_visible; }

	//モデルパス取得
	const std::string& GetModelPath()const { return model_path; }

	//モデルデータ取得
	std::shared_ptr<GltfModelData> GetModelData()const { return model_data; }

	//モデル取得
	GltfModel* GetModel()const { return model.get(); }

private:
	//モデルデータのロード処理
	bool LoadModel(const std::string& file_path);

private:
	std::shared_ptr<GltfModelData> model_data = nullptr;		//ModelManagerから共有キャッシュされるモデルデータ
	std::shared_ptr<GltfModelRenderer> renderer = nullptr;		//描画を司るレンダラー
	std::unique_ptr<GltfModel> model = nullptr;					//個別オブジェクトのモデル実体
	std::string model_path = "";								//ロードしているモデルファイルパス
	bool is_visible = true;										//描画可否フラグ
};

