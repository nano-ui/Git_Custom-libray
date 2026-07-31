#pragma once

#include <Windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

class ImGuiManager
{
public:
    //コンストラクタ
    ImGuiManager();

    //デストラクタ
    ~ImGuiManager();

    // コピー防止
    ImGuiManager(const ImGuiManager&) = delete;
    ImGuiManager& operator=(const ImGuiManager&) = delete;

    //初期化処理
    bool Initialize(HWND hwnd_window, ID3D11Device* device_ptr, ID3D11DeviceContext* context_ptr);

    //フレーム開始処理
    void BeginFrame();

    //フレーム描画処理
    void EndFrame();

    //解放処理
    void Shutdown();

private:
    //UIの初期スタイル設定
    void SetupStyle();

private:
    bool is_initialized;    //初期化呼び出しフラグ
};

