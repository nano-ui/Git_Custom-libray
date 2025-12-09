#include "DirectXDevice.h"
#include <d3d.h>
#include "misc.h"

////ウィンドウハンドルを受け取り、メンバ変数に保持
DirectXDevice::DirectXDevice(HWND window_handle)
	:window_handle_(window_handle)
{
}

//DirectX 11デバイス、即時コンテキスト、スワップチェーンを初期化
bool DirectXDevice::Initialize()
{
	HRESULT hr{ S_OK };

	//デバイス作成フラグの設定
	UINT create_device_flags{ 0 };

#ifdef _DEBUG
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;//デバッグビルドの場合、デバッグレイヤーを有効
#endif// _DEBUG

	//機能レベルの指定
	D3D_FEATURE_LEVEL feature_levels{ D3D_FEATURE_LEVEL_11_0 };//DirectX 11.0 の機能レベルを使用

	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	swap_chain_desc.BufferCount = 2;//バッファ数: 2
	swap_chain_desc.BufferDesc.Width = SCREEN_WIDTH;//縦のスクリーンサイズ
	swap_chain_desc.BufferDesc.Height = SCREEN_HEIGHT;//横のスクリーンサイズ
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//ピクセルフォーマット: 標準的なRGBA形式
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;//リフレッシュレート設定
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//バッファの使用方法: レンダーターゲットとして出力
	swap_chain_desc.OutputWindow = window_handle_;//出力ウィンドウの指定
	swap_chain_desc.SampleDesc.Count = 1;//マルチサンプリング設定: 1
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.Windowed = !FULLSCREEN;//ウィンドウモード/フルスクリーンの設定
	swap_chain_desc.Flags = 0;

	//デバイス、コンテキスト、スワップチェーンの作成
	hr = D3D11CreateDeviceAndSwapChain(
		NULL,								//描画アダプタの指定
		D3D_DRIVER_TYPE_HARDWARE,			//ハードウェアレンダリングを使用
		NULL,								//ソフトウェアラスタライザは使用しない
		create_device_flags,				//デバッグフラグなど
		&feature_levels,					//機能レベルの配列
		1,									//機能レベルの数
		D3D11_SDK_VERSION,					//SDKバージョン
		&swap_chain_desc,					//スワップチェーン設定
		swap_chain_.GetAddressOf(),			//[出力] スワップチェーン
		device_.GetAddressOf(),				//[出力] デバイス
		NULL,								//
		immediate_context_.GetAddressOf()	//[出力] 即時コンテキスト
	);

	//エラーチェック
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return SUCCEEDED(hr);
}

//レンダーターゲットビュー(RTV)と深度ステンシルビュー(DSV)を作成
void DirectXDevice::CreateRenderTargetAndDepthStencil()
{
}
