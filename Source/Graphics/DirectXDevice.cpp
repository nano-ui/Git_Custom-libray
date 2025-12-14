#include "DirectXDevice.h"
#include "misc.h"

////ウィンドウハンドルを受け取り、メンバ変数に保持
DirectXDevice::DirectXDevice(HWND window_handle)
	:window_handle_(window_handle)
{
}

//DirectX 11デバイス、即時コンテキスト、スワップチェーンを初期化
bool DirectXDevice::Initialize()
{
	HRESULT hr{ S_OK };//DirectXの関数の実行結果を格納

	UINT create_device_flags{ 0 };//デバイス作成時のオプション（デバッグ機能など）を設定する変数

#ifdef _DEBUG//デバッグビルド（開発中）の場合に限り、以下の処理を行う
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;//デバッグビルドの場合、デバッグレイヤーを有効
#endif// _DEBUG

	//機能レベルの指定
	D3D_FEATURE_LEVEL feature_levels{ D3D_FEATURE_LEVEL_11_0 };//DirectX 11.0 の機能レベルを使用

	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};							//スワップチェーン（画面表示の仕組み）の設定構造体
	swap_chain_desc.BufferCount = 2;								//描画バッファの数を2に設定
	swap_chain_desc.BufferDesc.Width = SCREEN_WIDTH;				//バッファの幅を設定
	swap_chain_desc.BufferDesc.Height = SCREEN_HEIGHT;				//バッファの高さを設定
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//色形式をRGBA各8ビットの標準的な形式に設定
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;			//リフレッシュレートの分子を 60 に設定
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;			//リフレッシュレートの分母を 1 に設定
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//バッファの用途を「描画先（レンダーターゲット）」として設定
	swap_chain_desc.OutputWindow = window_handle_;					//描画結果を出力するウィンドウを指定
	swap_chain_desc.SampleDesc.Count = 1;							//マルチサンプリングの数を 1（MSAAなし）に設定
	swap_chain_desc.SampleDesc.Quality = 0;							//アンチエイリアシング（MSAA）の品質レベルを設定
	swap_chain_desc.Windowed = !FULLSCREEN;							//ウィンドウモードを設定
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//コピー処理をなくして、効率よく画面を表示させる
	swap_chain_desc.Flags = 0;										//スワップチェーンの動作に関する特別なオプション（フラグ）を何も設定しない

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
		NULL,								//[出力] 機能レベル
		immediate_context_.GetAddressOf()	//[出力] 即時コンテキスト
	);

	//エラーチェック
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return SUCCEEDED(hr);					//初期化が成功したかどうかの結果を返す
}

//レンダーターゲットビュー(RTV)と深度ステンシルビュー(DSV)を作成
void DirectXDevice::CreateRenderTargetAndDepthStencil()
{
	HRESULT hr{ S_OK };

	//--------------------------------------
	//レンダーターゲットビュー (RTV) の作成
	//--------------------------------------

	//バックバッファのテクスチャオブジェクトのアクセス権限を取得
	Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;

	//スワップチェーンから実際に描画を行うためのテクスチャ（バックバッファ）を取得
	hr = swap_chain_->GetBuffer(
		0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<LPVOID*>(back_buffer.GetAddressOf())
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//レンダーターゲットビューを作成 (back_bufferを描画先として設定)
	hr = device_->CreateRenderTargetView(
		back_buffer.Get(), NULL, render_target_view_.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//--------------------------------------
	//深度ステンシルビュー (DSV) の作成
	//--------------------------------------

	//深度ステンシルバッファ用の2Dテクスチャを作成するための設定

	D3D11_TEXTURE2D_DESC texture2d_desc{};					//深度ステンシルバッファ（奥行き判定用）を作るための設定構造体
	texture2d_desc.Width = SCREEN_WIDTH;					//バッファの幅を設定
	texture2d_desc.Height = SCREEN_HEIGHT;					//バッファの高さを設定
	texture2d_desc.MipLevels = 1;							//テクスチャの解像度レベルの数を指定(1はミニマップを作成しない)
	texture2d_desc.ArraySize = 1;							//テクスチャが配列である場合の要素数を指定

	//深度・ステンシル用のピクセルフォーマット
	texture2d_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//深度情報24ビット、ステンシル情報8ビットというピクセル形式を指定
	texture2d_desc.SampleDesc.Count = 1;					//アンチエイリアシング（MSAA）を行う際のサンプルの数を指定(1はMASSを使用しない)
	texture2d_desc.SampleDesc.Quality = 0;					//MSAAの品質レベルを指定(Countが1の場合、この値は0に設定)
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;				//のテクスチャリソース（深度バッファ）が、GPUから読み書きされる（標準的な）使い方をされることを指定

	//バインドフラグ: 深度ステンシルバッファとして使用
	texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;	//このテクスチャを「深度ステンシルバッファ」として使うことを指定
	texture2d_desc.CPUAccessFlags = 0;						//このテクスチャは主にGPUだけが使用し、CPUは読み書きしない
	texture2d_desc.MiscFlags = 0;							//このテクスチャには特別な用途はなく、標準的なテクスチャとして使用

	//深度ステンシルバッファ (テクスチャ) の作成
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_stencil_buffer;//深度ステンシルバッファ本体を格納

	//設定に基づいて深度ステンシルバッファをGPUメモリ上に作成
	hr = device_->CreateTexture2D(
		&texture2d_desc, NULL, depth_stencil_buffer.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//深度ステンシルビューを作成するための設定
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stancil_view_desc{};				//深度ステンシルバッファを、コンテキストに設定するためのビュー設定構造体
	depth_stancil_view_desc.Format = texture2d_desc.Format;					//ビュー（深度バッファへのアクセス方法）のピクセル形式を、元となるテクスチャ（texture2d_desc）の形式と一致させる
	depth_stancil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;	//このリソースを2Dテクスチャとして扱うビューであることを指定
	depth_stancil_view_desc.Texture2D.MipSlice = 0;							//ミップマップが複数ある場合、どのレベル（スライス）をビューとして使用するかを指定(0はフル解像度)

	//作成した深度ステンシルバッファのビュー（DSV）を作成
	hr = device_->CreateDepthStencilView(
		depth_stencil_buffer.Get(),
		&depth_stancil_view_desc,
		depth_stencil_view_.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//DSVの作成が成功したかを確認
}
