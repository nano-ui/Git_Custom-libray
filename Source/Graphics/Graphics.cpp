#include "Graphics.h"

static constexpr UINT constant_buffer_slot_scene = 1;

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

    if (!CreateSceneConstantBuffer())
    {
        return false;
    }

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
    scene_constant_buffer.Reset();
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

//シーン定数バッファを生成
bool Graphics::CreateSceneConstantBuffer()
{
    //定数バッファの設定とリソース生成
    D3D11_BUFFER_DESC buffer_desc{};                    //DirectX11のバッファ設定用構造体を初期化宣言
    buffer_desc.ByteWidth = sizeof(scene_constants);    //転送する構造体のサイズをバイト単位で正確に指定
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;            //CPUから書き換え可能かつGPUからアクセスできる標準的な設定
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //パイプラインに定数バッファとして結びつけるフラグを設定します
    buffer_desc.CPUAccessFlags = 0;                     //UsageがDEFAULTのためCPUからの直接アクセスフラグは不要なので0
    buffer_desc.MiscFlags = 0;                          //その他の特殊なバッファオプションは使用しないため0
    buffer_desc.StructureByteStride = 0;                //構造化バッファではないためストライド値は0

    HRESULT hr = GetDevice()->CreateBuffer(&buffer_desc, nullptr, scene_constant_buffer.GetAddressOf());

    if (FAILED(hr))
    {
        return false; // 生成に失敗した場合は偽を返して終了します
    }

    return true;
}

//シーン定数バッファを更新
void Graphics::UpdateSceneConstantBuffer(const scene_constants& constants)
{
    if (!scene_constant_buffer)
    {
        return; //生成されていなければ処理を終了
    }

    //GPUバッファへのデータコピーと各シェーダーへの設定
    ID3D11DeviceContext* context = GetContext();
    context->UpdateSubresource(scene_constant_buffer.Get(), 0, nullptr, &constants, 0, 0);
    context->VSSetConstantBuffers(constant_buffer_slot_scene, 1, scene_constant_buffer.GetAddressOf());
    context->PSSetConstantBuffers(constant_buffer_slot_scene, 1, scene_constant_buffer.GetAddressOf());
}
