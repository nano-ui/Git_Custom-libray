#pragma once

#include "SequenceSceneBace.h"

#include <memory>
#include <string>

class framebuffer;
class Camera;
class Model;

struct ID3D11DeviceContext;

class ModelPreviewScene : public SequenceSceneBace
{
public:
	//コンストラクタ
	ModelPreviewScene();

	//デストラクタ
	~ModelPreviewScene()override = default;

	//初期化
	void Initialize()override;

	//更新
	void Update(float elapsed_time)override;

	//描画
	void Render(ID3D11DeviceContext* immediate_context)override;

	//ImGui描画
	void RenderGui()override;

	//モデル読み込み
	void LoadModel(const std::string& file_path);

private:
	std::unique_ptr<framebuffer> frame_buffer;	//描画バッファ
	std::unique_ptr<Camera> camera;				//カメラ
	std::unique_ptr<Model> model;				//プレビュー用モデル
};

