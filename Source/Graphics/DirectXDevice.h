#pragma once

#include <d3d11.h>
#include <wrl.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define FULLSCREEN FALSE

class DirectXDevice
{
public:
	//ウィンドウハンドルを受け取り、メンバ変数に保持
	DirectXDevice(HWND window_handle);

	//DirectX 11デバイス、即時コンテキスト、スワップチェーンを初期化
	bool Initialize();

	//レンダーターゲットビュー(RTV)と深度ステンシルビュー(DSV)を作成
	void CreateRenderTargetAndDepthStencil();

	//ID3D11Deviceオブジェクト（GPUとのインターフェース）を取得
	Microsoft::WRL::ComPtr<ID3D11Device>& GetDevice() { return device_; }

	//ID3D11DeviceContextオブジェクト（GPUへの描画コマンド発行）を取得
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetImmediateContext() { return immediate_context_; }

	//IDXGISwapChainオブジェクト（描画結果の表示管理）を取得
	Microsoft::WRL::ComPtr<IDXGISwapChain>& GetSwapChain() { return swap_chain_; }

	//ID3D11RenderTargetViewオブジェクト（バックバッファへの描画先）を取得
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& GetRenderTargetView() { return render_target_view_; }

	//ID3D11DepthStencilViewオブジェクト（深度・ステンシルテスト用バッファ）を取得
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& GetDepthStencilView() { return depth_stencil_view_; }

private:
	const HWND window_handle_;											//描画対象のウィンドウハンドル
	Microsoft::WRL::ComPtr<ID3D11Device> device_;						//GPUとの接続とリソース（バッファ、テクスチャなど）の作成
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context_;		//GPUへの描画コマンド（描画設定、描画実行など）の発行
	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;					//描画結果（バックバッファ）を画面に表示
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;	//バックバッファ（描画先のテクスチャ）にアクセスするためのビュー
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view_;	//深度テストとステンシルテストに使用するバッファにアクセスするためのビュー
};

