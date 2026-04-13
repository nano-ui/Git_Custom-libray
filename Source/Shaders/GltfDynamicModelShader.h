#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <memory>

#include "../GlthModel/GlthDynamicModel/GltfDynamicModelData.h"

class GltfDynamicModelShader
{
public:

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;   // 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;     // ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;     // 入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11SamplerState> common_sampler;  // サンプラーステート
	Microsoft::WRL::ComPtr<ID3D11Buffer> scene_cb;              // シーン用定数バッファ
};

