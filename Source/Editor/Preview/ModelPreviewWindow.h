#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

class framebuffer;
class Camera;
class Model;
class SkyBox;
class ShapeRenderer;

struct ID3D11DeviceContext;

class ModelPreviewWindow
{
public:
	//コンストラクタ
	ModelPreviewWindow();

	//デストラクタ
	~ModelPreviewWindow();

	//初期化処理
	void Initialize();

	//更新処理
	void Update(float elapsed_time);

	//描画処理
	void Render(ID3D11DeviceContext* immediate_context);

	//ImGui描画
	void RenderGui();

	//外部からモデル読み込み
	void LoadModel(const std::string& file_path);

private:
	//UIコントロール描画
	void DrawControlPanel();

private:
	std::unique_ptr<framebuffer> frame_buffer;		//描画結果を保存
	std::unique_ptr<Camera> camera;					//フリーカメラ
	std::unique_ptr<Model> model;					//3Dモデル
	std::unique_ptr<SkyBox> skybox;					//スカイボックス
	std::unique_ptr<ShapeRenderer> shape_renderer;	//デバッグ描画

	std::string current_model_path = "";		//現在のモデルパス
	std::vector<std::string> animation_names;	//アニメーションリスト

	DirectX::XMFLOAT3 model_position = { 0.0f, 0.0f, 0.0f };//座標
	DirectX::XMFLOAT3 model_rotation = { 0.0f,0.0f,0.0f };	//角度
	DirectX::XMFLOAT3 model_scale = { 1.0f,1.0f,1.0f };		//拡大率
	bool is_loop = true;									//ループフラグ
	int selected_animation_index = -1;						//選択中のアニメーションインデックス
	bool is_viewport_active = false;						//カメラ操作フラグ

	bool show_grid = true;										//グリッドの表示・非表示フラグ
	float grid_size = 10.0f;									//グリッドの全体サイズ
	int grid_divisions = 10;									//グリッドの分割数
	DirectX::XMFLOAT4 grid_color = { 1.0f, 1.0f, 1.0f, 1.0f };	//グリッドの色
};

