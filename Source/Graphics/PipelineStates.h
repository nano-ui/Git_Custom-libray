#pragma once

#include <d3d11.h>
#include <wrl.h>

class PipelineStates
{
public:
	//ID3D11Deviceポインタを受け取り、ステート生成に備える
	PipelineStates(ID3D11Device* device);

	//全てのパイプラインステート（Sampler, DepthStencil, Blend, Rasterizer）を作成
	void Initialize();

	//指定されたインデックスのサンプラーステートを取得
	//インデックス 0: リピート, 1: クランプ, 2: ミラー
	Microsoft::WRL::ComPtr<ID3D11SamplerState>& GetSamplerState(size_t index) { return sampler_states_[index]; }

	//指定されたインデックスの深度ステンシルステートを取得
	//インデックス 0: 無効, 1: 有効, 2: 深度テストのみ有効, 3: 深度書き込み無効
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& GetDepthStenceilState(size_t index) { return depth_stencil_states_[index];}
	
	//指定されたインデックスのブレンドステートを取得
	//インデックス 0: ブレンド無効, 1: アルファブレンド, 2: 加算ブレンド, 3: 乗算ブレンド
	Microsoft::WRL::ComPtr<ID3D11BlendState>& GetBlendState(size_t index) { return blend_states_[index]; }

	//指定されたインデックスのラスタライザステートを取得
	//インデックス 0: デフォルト, 1: ワイヤーフレーム, 2: 裏面カリングなし
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>& GetRasterizerState(size_t index) { return rasterizer_states_[index]; }

private:
	ID3D11Device* device_ptr_;													//ステート作成に使用するID3D11Deviceへのポインタ。ステートの生成時のみ使用
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_states_[3];				//テクスチャのサンプリング方法（リピート、クランプなど）を管理するステート配列
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_states_[4];	//深度テストとステンシルテストの有効/無効、書き込み方法などを管理するステート配列
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states_[4];					//ピクセル色の合成（アルファブレンド、加算など）を管理するステート配列
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_states_[3];		//ポリゴンの塗りつぶし方法（ソリッド/ワイヤーフレーム）やカリング（裏面削除）を管理するステート配列
};

