#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <memory>

#include "../GlthModel/GlthDynamicModel/GltfDynamicModelData.h"

class GltfDynamicModelShader
{
public:
	GltfDynamicModelShader();

	//初期化
	HRESULT Initialize(ID3D11Device* device);

	//シーン定数バッファ更新
	void UpdateSceneBuffer(ID3D11DeviceContext* dc, const SceneConstantBuffer& scene_data);

	//シェーダーのバインド
	void Bind(ID3D11DeviceContext* dc);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;   // 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;     // ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;     // 入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11SamplerState> common_sampler;  // サンプラーステート
	Microsoft::WRL::ComPtr<ID3D11Buffer> scene_cb;              // シーン用定数バッファ
};

