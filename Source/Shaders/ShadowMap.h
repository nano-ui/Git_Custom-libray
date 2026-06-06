#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

#include "CustomShader.h"

class ShadowMap
{
public:
	//コンストラクタ
	ShadowMap();

	//デストラクタ
	~ShadowMap();

	//初期化
	void Initialize(ID3D11Device* device);

	//影の書き込みパス開始処理（光源からの深度描画）
	void BeginCasterPass(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& light_direction);

	//影の読み込みパス開始処理（通常のオブジェクト描画へのバインド）
	void BindReceiverPass(ID3D11DeviceContext* context, UINT texture_slot, UINT sampler_slot, UINT constant_buffer_slot);

	//影の読み込みパス終了処理（テクスチャの強制バインド解除）
	void UnbindReceiverPass(ID3D11DeviceContext* context, UINT texture_slot);

	//ImGuiデバッグリソースビュー取得
	ID3D11ShaderResourceView* GetShaderResourceView()const { return shader_resource_view.Get(); }

	//影の色取得
	DirectX::XMFLOAT3 GetShadowColor()const { return shadow_color; }

	//影の色設定
	void SetShadowColor(const DirectX::XMFLOAT3& color) { shadow_color = color; }

	//影のバイアス取得
	float GetShadowBias()const { return shadow_bias; }

	//影のバイアス設定
	void SetShadowBias(float bias) { shadow_bias = bias; }

private:
	//シャドウマップ用定数バッファ
	struct shadowmap_constants
	{
		DirectX::XMFLOAT4X4 light_view_projection;	//ライトの位置から見た射影行列
		DirectX::XMFLOAT3 shadow_color;				//影の色
		float shadow_bias;							//深度バイアス
	};

	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view;		//深度書き込み用ビュー
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;	//深度書き込み用テクスチャ
	Microsoft::WRL::ComPtr<ID3D11VertexShader> caster_vertex_shader;		//影描画用頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> caster_input_layout;			//影描画専用頂点レイアウト
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;					//影計算用定数バッファ
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state;				//影用のサンプラステート

	DirectX::XMFLOAT4X4 light_view_projection;	//ライトビュープロジェクション行列
	DirectX::XMFLOAT3 shadow_color;				//影の色成分
	float shadow_bias;							//深度バイアス値

	CustomShader shadow_caster_shader;			//シェーダー管理コンポーネント
};

