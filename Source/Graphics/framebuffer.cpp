#include "framebuffer.h"
#include "misc.h"

framebuffer::framebuffer(ID3D11Device* device, uint32_t width, uint32_t height)
{
	HRESULT hr{ S_OK };

	//テクスチャ用の変数を用意
	Microsoft::WRL::ComPtr<ID3D11Texture2D> render_target_buffer;

	//テクスチャの仕様を設定
	D3D11_TEXTURE2D_DESC texture2d_desc{};
	texture2d_desc.Width = width;	//横幅（ピクセル数）
	texture2d_desc.Height = height; //縦幅（ピクセル数）
	texture2d_desc.MipLevels = 1;	//ミップマップのレベル数（1 = ミップマップなし）
	texture2d_desc.ArraySize = 1;	//配列の要素数
	texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;	//テクスチャの色フォーマット
	texture2d_desc.SampleDesc.Count = 1;//マルチサンプリング数
	texture2d_desc.SampleDesc.Quality = 0;//品質
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;//GPUで読み書きする
	texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;//テクスチャは「描画先（RTV）」にも「シェーダから読む（SRV）」にも使う
	texture2d_desc.CPUAccessFlags = 0;//CPUから直接アクセスしない
	texture2d_desc.MiscFlags = 0;//特殊なオプションなし

	//実際にテクスチャを作成
	hr = device->CreateTexture2D(&texture2d_desc, 0, render_target_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック

	//テクスチャ(render_target_buffer)を、描画先として使えるようにするビュー(RenderTargetView)を作成
	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc{};
	render_target_view_desc.Format = texture2d_desc.Format;//さっき作ったテクスチャのフォーマットと同じものを指定
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;//どんな形のリソースを対象にするかを指定(普通の2Dテクスチャ)
	
	//Render Target View の生成
	hr = device->CreateRenderTargetView(render_target_buffer.Get(), &render_target_view_desc,
		          render_target_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック

	//シェーダーリソースビューの設定を準備
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
	shader_resource_view_desc.Format = texture2d_desc.Format;//シェーダーに渡すときのデータ形式を指定
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;//どんな形のリソースを対象にするか(2Dテクスチャ)
	shader_resource_view_desc.Texture2D.MipLevels = 1;//ミップマップの数を指定

	//Shader Resource View の生成
	hr = device->CreateShaderResourceView(render_target_buffer.Get(), &shader_resource_view_desc,
		shader_resource_views[0].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック

	//深度ステンシル用のテクスチャを作る処理
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_stencil_buffer;
	texture2d_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;//テクスチャのフォーマットを設定
	texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;//「深度バッファ」としても使えるし「シェーダーの入力」としても使える
	
	//実際にテクスチャを作成
	hr = device->CreateTexture2D(&texture2d_desc, 0, depth_stencil_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック

	//DSV の設定
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//このテクスチャを DSV として使うときは、深度+ステンシルとする
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャとして使う
	depth_stencil_view_desc.Flags = 0;//特殊な設定なし

	//DSV の作成
	hr = device->CreateDepthStencilView(depth_stencil_buffer.Get(), &depth_stencil_view_desc,
		depth_stencil_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック

	shader_resource_view_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;//シェーダーから深度値だけを読み取りたいので、SRV側では深度を R チャネルとして取り出せるように指定
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;//このSRVは2Dテクスチャを扱うビューです」と宣言

	//実際に SRV オブジェクトを作成
	hr = device->CreateShaderResourceView(depth_stencil_buffer.Get(), &shader_resource_view_desc,
	          shader_resource_views[1].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック

	viewport.Width = static_cast<float>(width);//ビューポートの横幅を設定
	viewport.Height = static_cast<float>(height);//ビューポートの高さを設定
	viewport.MinDepth = 0.0f;//深度（Z値）の最小値を設定
	viewport.MaxDepth = 1.0f;//深度（Z値）の最大値を設定
	viewport.TopLeftX = 0.0f;//ビューポートの左上の X 座標(画面の一番左から描画を始める)
	viewport.TopLeftY = 0.0f;//ビューポートの左上の Y 座標(画面の一番上から描画を始める)
}

//初期化
void framebuffer::clear(ID3D11DeviceContext* immediate_context,
	float r, float g, float b, float a, float depth)
{
	float color[4]{ r,g,b,a };//色情報
	immediate_context->ClearRenderTargetView(render_target_view.Get(), color);//render_target_view に指定された描画対象(通常は画面)を、colorで塗りつぶし
	immediate_context->ClearDepthStencilView(depth_stencil_view.Get(), D3D11_CLEAR_DEPTH, depth, 0);//Zバッファをリセットして「全ピクセルを描画されていない状態」にする
}

//「現在の描画先」の設定
void framebuffer::activate(ID3D11DeviceContext* immediate_context)
{
	viewport_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;//Direct3D11が一度に扱える最大ビューポート数を設定
	immediate_context->RSGetViewports(&viewport_count, cached_viewports);//現在のレンダリングコンテキストに設定されているビューポート情報取得
	
	/*現在設定されているレンダーターゲットビュー（描画先テクスチャ）と深度ステンシルビューを取得して保存*/
	immediate_context->OMGetRenderTargets(1, cached_render_target_view.ReleaseAndGetAddressOf(),
		         cached_depth_stencil_view.ReleaseAndGetAddressOf());

	immediate_context->RSSetViewports(1, &viewport);//framebufferオブジェクトが持っているビューポートをレンダリングコンテキストに設定
	
	//保存しておいたレンダーターゲットと深度ステンシルを再びセット
	immediate_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(),
		depth_stencil_view.Get());
}

//activateで切り替えた描画先を元に戻す
void framebuffer::deactivate(ID3D11DeviceContext* immediate_context)
{
	immediate_context->RSSetViewports(viewport_count, cached_viewports);//フレームバッファを使う前の描画領域に戻す

	/*framebufferに切り替えた描画先から、元の描画先（通常はバックバッファなど)に戻す*/
	immediate_context->OMSetRenderTargets(1, cached_render_target_view.GetAddressOf(),
		cached_depth_stencil_view.Get());

	cached_render_target_view.Reset();
	cached_depth_stencil_view.Reset();
}
