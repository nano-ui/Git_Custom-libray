#include "Graphics.h"

//シングルトンインスタンス取得
Graphics& Graphics::Instance()
{
    static Graphics instance;
    return instance;
}

//初期化
bool Graphics::Initialize(HWND window_handle)
{
    //デバイスを作成
    directx_device = std::make_unique<DirectXDevice>(window_handle);
    if (!directx_device->Initialize())
    {
        return false;
    }

    //レンダーターゲット、深度ステンシルビューの作成
    directx_device->CreateRenderTargetAndDepthStencil();

    //作成したデバイスを使い、パイプラインステートを作成
    pipline_states = std::make_unique<PipelineStates>(GetDevice());
    pipline_states->Initialize();

    return true;
}

//明示的にスマートポインタを解放したい場合
void Graphics::Finalize()
{
    if (directx_device)
    {
        auto context = directx_device->GetImmediateContext();
        if (context)
        {
            context->ClearState();  //全ての設定をリセット
            context->Flush();       //コマンドを強制実行して完了させる
        }
    }

    //スマートポインタの開放
    pipline_states.reset();
    directx_device.reset();
}

//描画開始
void Graphics::BeginFrame(float r, float g, float b, float a)
{
    auto context = directx_device->GetImmediateContext();
    auto rtv = directx_device->GetRenderTargetView();
    auto dsv = directx_device->GetDepthStencilView();

    //描画のクリア色設定
    float clear_color[] = { r,g,b,a };

    //レンダーターゲットのクリア
    context->ClearRenderTargetView(rtv.Get(), clear_color);

    //深度・ステンシルバッファのクリア
    context->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //レンダーターゲットのセット
    ID3D11RenderTargetView* rtv_list[] = { rtv.Get() };
    context->OMSetRenderTargets(1, rtv_list, dsv.Get());

    //ビューポートの設定
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(SCREEN_WIDTH);
    viewport.Height = static_cast<float>(SCREEN_HEIGHT);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context->RSSetViewports(1, &viewport);
}

//描画終了
void Graphics::EndFrame()
{
    //垂直同期待機
    directx_device->GetSwapChain()->Present(0, 0);
}
