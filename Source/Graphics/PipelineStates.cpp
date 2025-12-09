#include "PipelineStates.h"
#include "misc.h"

//ID3D11Deviceポインタを受け取り、ステート生成に備える
PipelineStates::PipelineStates(ID3D11Device* device)
	:device_ptr_(device)
{
}

//全てのパイプラインステート（Sampler, DepthStencil, Blend, Rasterizer）を作成
void PipelineStates::Initialize()
{
	HRESULT hr{ S_OK };

	//-------------------------
	//サンプラーステートの作成
	//-------------------------

	//リピート(INDEX : 0)
	D3D11_SAMPLER_DESC sampler_desc_repeat{};
	sampler_desc_repeat.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;	//
	sampler_desc_repeat.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;					//リピート
	sampler_desc_repeat.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;					//リピート
	sampler_desc_repeat.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;					//リピート
	sampler_desc_repeat.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc_repeat.MinLOD = 0;
	sampler_desc_repeat.MaxLOD = D3D11_FLOAT32_MAX;

	//サンプラーステートオブジェクトを作成
	hr = device_ptr_->CreateSamplerState(
		&sampler_desc_repeat, sampler_states_[0].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//クランプ(INDEX : 1)
	D3D11_SAMPLER_DESC sampler_desc_clamp = sampler_desc_repeat;
	sampler_desc_clamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;		//クランプ(端の色で固定)
	sampler_desc_clamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;		//クランプ(端の色で固定)
	sampler_desc_clamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;		//クランプ(端の色で固定)

	//サンプラーステートオブジェクトを作成
	hr = device_ptr_->CreateSamplerState(
		&sampler_desc_clamp, sampler_states_[1].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//ミラー(INDEX : 2)
	D3D11_SAMPLER_DESC sampler_desc_mirror = sampler_desc_repeat;
	sampler_desc_mirror.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;	//ミラー(反転して繰り返し)
	sampler_desc_mirror.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;	//ミラー(反転して繰り返し)
	sampler_desc_mirror.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;	//ミラー(反転して繰り返し)

	//サンプラーステートオブジェクトを作成
	hr = device_ptr_->CreateSamplerState(
		&sampler_desc_mirror, sampler_states_[2].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//-----------------------------
	//深度ステンシルステートの作成
	//-----------------------------

	//深度・ステンシル無効　(INDEX : 0)
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc_disabled{};
	depth_stencil_desc_disabled.DepthEnable = FALSE;							//深度テストを無効化
	depth_stencil_desc_disabled.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	//深度書き込みを無効化
	depth_stencil_desc_disabled.DepthFunc = D3D11_COMPARISON_ALWAYS;
	
	//深度ステンシルステートオブジェクトを作成
	hr = device_ptr_->CreateDepthStencilState(
		&depth_stencil_desc_disabled, depth_stencil_states_[0].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//深度・ステンシル有効　(INDEX : 1)
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc_enabled{};
	depth_stencil_desc_enabled.DepthEnable = TRUE;								//深度テストを有効化
	depth_stencil_desc_enabled.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;		//深度書き込みを有効化
	depth_stencil_desc_enabled.DepthFunc = D3D11_COMPARISON_LESS;				//手前にあるピクセルを描画
	depth_stencil_desc_enabled.StencilEnable = FALSE;

	//深度ステンシルステートオブジェクトを作成
	hr = device_ptr_->CreateDepthStencilState(
		&depth_stencil_desc_enabled, depth_stencil_states_[1].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//深度テストのみ有効（深度書き込みは無効)　(INDEX　: 2)
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc_no_write = depth_stencil_desc_enabled;
	depth_stencil_desc_no_write.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	//深度書き込みを無効化

	//深度ステンシルステートオブジェクトを作成
	hr = device_ptr_->CreateDepthStencilState(
		&depth_stencil_desc_no_write, depth_stencil_states_[2].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//深度書き込み無効、深度テストは等しい場合のみパス　(INDEX　: 3)
	D3D11_DEPTH_STENCIL_DESC depth_stencile_desc_equal{};
	depth_stencile_desc_equal.DepthEnable = TRUE;								//深度テストを無効化
	depth_stencile_desc_equal.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;		//深度書き込みを無効化
	depth_stencile_desc_equal.DepthFunc = D3D11_COMPARISON_EQUAL;				//深度値が等しい場合のみパス

	//深度ステンシルステートオブジェクトを作成
	hr = device_ptr_->CreateDepthStencilState(
		&depth_stencile_desc_equal, depth_stencil_states_[3].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//-----------------------
	//ブレンドステートの作成
	//-----------------------

	//ブレンドの無効 (INDEX : 0)
	D3D11_BLEND_DESC blend_desc_disabled{};



}
