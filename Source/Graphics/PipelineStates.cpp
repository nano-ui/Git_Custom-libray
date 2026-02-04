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
	D3D11_SAMPLER_DESC sampler_desc_repeat{};									//設定構造体を初期化
	sampler_desc_repeat.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;;				//テクセル（テクスチャのピクセル）を補間する方法（ここでは線形補間）を設定
	sampler_desc_repeat.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;					//U座標（横方向）が範囲外の場合、テクスチャを繰り返し
	sampler_desc_repeat.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;					//V座標（縦方向）が範囲外の場合、テクスチャを繰り返し
	sampler_desc_repeat.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;					//W座標（奥行きや、テクスチャ配列のインデックス）が範囲外の場合、テクスチャを繰り返し
	sampler_desc_repeat.ComparisonFunc = D3D11_COMPARISON_NEVER;				//シャャドウマップなどの比較サンプリングを行うときに使う機能ですが、ここでは比較を一切行わない (NEVER) と設定
	sampler_desc_repeat.MinLOD = 0;												//最も解像度の高いオリジナルのテクスチャを設定
	sampler_desc_repeat.MaxLOD = D3D11_FLOAT32_MAX;								//テクスチャに用意されている全てのミップマップレベルを使用可能

	//サンプラーステートオブジェクトを作成
	hr = device_ptr_->CreateSamplerState(
		&sampler_desc_repeat, sampler_states_[0].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//クランプ(INDEX : 1)
	D3D11_SAMPLER_DESC sampler_desc_clamp = sampler_desc_repeat;
	sampler_desc_clamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;		//範囲外の座標を端のテクセルで固定
	sampler_desc_clamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;		//範囲外の座標を端のテクセルで固定
	sampler_desc_clamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;		//範囲外の座標を端のテクセルで固定

	//サンプラーステートオブジェクトを作成
	hr = device_ptr_->CreateSamplerState(
		&sampler_desc_clamp, sampler_states_[1].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//ミラー(INDEX : 2)
	D3D11_SAMPLER_DESC sampler_desc_mirror = sampler_desc_repeat;
	sampler_desc_mirror.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;	//範囲外の座標を反転させて繰り返し
	sampler_desc_mirror.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;	//範囲外の座標を反転させて繰り返し
	sampler_desc_mirror.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;	//範囲外の座標を反転させて繰り返し

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
	depth_stencil_desc_disabled.DepthFunc = D3D11_COMPARISON_ALWAYS;			//深度テストの比較条件を常にTRUEに設定
	
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
	depth_stencil_desc_enabled.StencilEnable = FALSE;							//ステンシルテストを無効化

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

	//深度書き込み行こうか、深度テストは等しい場合のみパス　(INDEX　: 3)
	D3D11_DEPTH_STENCIL_DESC depth_stencile_desc_equal{};
	depth_stencile_desc_equal.DepthEnable = TRUE;								//深度テストを有効化
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
	blend_desc_disabled.RenderTarget[0].BlendEnable = FALSE;	//ブレンド無効
	blend_desc_disabled.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;//レンダリングターゲット（描画先のバッファ）に対して、RGBAの全チャンネルを書き込むことを許可する

	//ブレンドステートオブジェクトを作成
	hr = device_ptr_->CreateBlendState(
		&blend_desc_disabled, blend_states_[0].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//アルファブレンド (INDEX : 1)
	D3D11_BLEND_DESC blend_desc_alpa = blend_desc_disabled;
	blend_desc_alpa.RenderTarget[0].BlendEnable = TRUE;						//ブレンド有効
	blend_desc_alpa.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;	//新しい色のアルファ値を使用
	blend_desc_alpa.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	//既に描画されている色の (1 - アルファ値) を使用
	blend_desc_alpa.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc_alpa.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_desc_alpa.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc_alpa.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	//ブレンドステートオブジェクトを作成
	hr = device_ptr_->CreateBlendState(
		&blend_desc_alpa, blend_states_[1].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//加算ブレンド (INDEX : 2)
	D3D11_BLEND_DESC blend_desc_add = blend_desc_alpa;
	blend_desc_add.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;	//新しい色のアルファ値を使用
	blend_desc_add.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;			//既に描画されている色をそのまま残す（加算）

	//ブレンドステートオブジェクトを作成
	hr = device_ptr_->CreateBlendState(
		&blend_desc_add, blend_states_[2].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//乗算ブレンド (INDEX : 3)
	D3D11_BLEND_DESC blend_desc_mul = blend_desc_alpa;
	blend_desc_mul.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;	//既存の色を使用
	blend_desc_mul.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;		//新しい色を乗算（Dest=0なので新しい色*既存色）
	
	//ブレンドステートオブジェクトを作成
	hr = device_ptr_->CreateBlendState(
		&blend_desc_mul, blend_states_[3].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//----------------------------
	//ラスタライザステートの作成
	//----------------------------

	//デフォルト (INDEX : 0)
	D3D11_RASTERIZER_DESC rasterizer_desc_default{};
	rasterizer_desc_default.FillMode = D3D11_FILL_SOLID;	//ポリゴンをソリッドで（中を塗りつぶして）描画
	rasterizer_desc_default.CullMode = D3D11_CULL_BACK;		//裏面カリング有効化（裏側のポリゴンを描画しない）
	rasterizer_desc_default.FrontCounterClockwise = TRUE;
	rasterizer_desc_default.DepthClipEnable = TRUE;			//深度クリッピングを有効化
	
	//ラスタライザステートオブジェクトを作成
	hr = device_ptr_->CreateRasterizerState(
		&rasterizer_desc_default, rasterizer_states_[0].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//ワイヤーフレーム (INDEX : 1)
	D3D11_RASTERIZER_DESC rasterizer_desc_wireframe = rasterizer_desc_default;
	rasterizer_desc_wireframe.FillMode = D3D11_FILL_WIREFRAME;	//塗りつぶしをワイヤーフレーム(ポリゴンの辺のみを描画)に変更

	//ラスタライザステートオブジェクトを作成
	hr = device_ptr_->CreateRasterizerState(
		&rasterizer_desc_wireframe, rasterizer_states_[1].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//カリング無し (INDEX : 2)
	D3D11_RASTERIZER_DESC rasterizer_desc_no_cull = rasterizer_desc_default;
	rasterizer_desc_no_cull.CullMode = D3D11_CULL_NONE;							//カリング(裏面削除)を無効化

	//ラスタライザステートオブジェクトを作成
	hr = device_ptr_->CreateRasterizerState(
		&rasterizer_desc_no_cull, rasterizer_states_[2].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}
