#pragma once
#include "../Shaders/PostProcess.h"
class BasicShader : public PostProcess
{
public:
	//コンストラクタ
	BasicShader(const std::string& shader_filename) :filename(shader_filename) {};

	//デストラクタ
	~BasicShader()override = default;

	//シェーダー生成
	void Initialize(ID3D11Device* device)override;

	//シェーダーを適用して描画
	void Render(
		ID3D11DeviceContext* context,
		ID3D11ShaderResourceView** source_srv,
		ID3D11RenderTargetView* dest_rtv,
		fullscreen_quad* quad) override;

private:
	std::string filename;	//読み込むシェーダーのファイル名
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;	//ピクセルシェーダー
};

