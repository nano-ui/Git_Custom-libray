#include "ShaderManager.h"
#include "GltfDynamicModelShader.h"

//============================
//全シェーダーの初期化
//============================
HRESULT ShaderManager::Initialize(ID3D11Device* device)
{
    //実行結果保存
    HRESULT hr = S_OK;

    //GltfDynamicModel用シェーダーの生成と初期化
    gltf_dynamic_model_shader = std::make_unique<GltfDynamicModelShader>();

    //リソースを生成
    hr = gltf_dynamic_model_shader->Initialize(device);

    //失敗した場合はエラーを返して中断
    if (FAILED(hr))
    {
        return hr;
    }

    return hr;
}
