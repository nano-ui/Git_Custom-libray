#pragma once
#include "../Shaders/PostProcess.h"
#include "../Graphics/PostProcessConstantBuffers.h"

class LuminanceExtractionEffect : public PostProcess
{
public:
	//コンストラクタ
	LuminanceExtractionEffect(float threshold = 1.0f);

	//デストラクタ
	~LuminanceExtractionEffect()override = default;

	//初期化
	void Initialize(ID3D11Device* device)override;

	//描画処理
	void Render(
		ID3D11DeviceContext* context,
		ID3D11ShaderResourceView** source_srv,
		ID3D11RenderTargetView* dest_rtv,
		fullscreen_quad* quad) override;

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer_;
	float threshold;
};

