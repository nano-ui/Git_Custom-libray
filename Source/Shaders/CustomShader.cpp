#include "CustomShader.h"
#include "Graphics.h"
#include "shader.h"

#include <filesystem>
#include <vector>
#include <memory>
#include "BlurShader.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

//マジックナンバーを避けるための定数定義群
static constexpr UINT default_semantic_index = 0;
static constexpr UINT default_input_slot = 0;
static constexpr UINT default_instance_data_step_rate = 0;
static constexpr UINT file_seek_start = 0;
static constexpr size_t read_count = 1;
static constexpr BYTE mask_x = 1;
static constexpr BYTE mask_xy = 3;
static constexpr BYTE mask_xyz = 7;
static constexpr BYTE mask_xyzw = 15;

//====================================================
//指定されたファイル名からシェーダーを生成し初期化
//====================================================
bool CustomShader::Initialize(
    const std::string& vs_name,                     //頂点シェーダーファイル名
    const std::string& ps_name                     //ピクセルシェーダーファイル名
)
{
    //頂点シェーダーと入力レイアウトの生成
    if (!CreateVertexShaderWithReflection(vs_name))
    {
        return false;
    }

    //初期化準備
    auto device = Graphics::Instance().GetDevice();

    //ピクセルシェーダーの生成
    HRESULT hr = create_ps_from_cso(
        device,
        ps_name.c_str(),
        pixel_shader.GetAddressOf()
    );
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

//==================================================================
//保持しているシェーダーと入力レイアウトをパイプラインにバインド
//==================================================================
void CustomShader::Apply()
{
    //バインド準備
    auto context = Graphics::Instance().GetContext();

    //パイプラインへの適用
    context->IASetInputLayout(input_layout.Get());
    context->VSSetShader(vertex_shader.Get(), nullptr, 0);
    context->PSSetShader(pixel_shader.Get(), nullptr, 0);
}

//=====================================================================
//頂点シェーダーを読み込み、リフレクションで入力レイアウトを自動生成
//=====================================================================
bool CustomShader::CreateVertexShaderWithReflection(const std::string& vs_name)
{
    //ファイル読み込み
    std::filesystem::path file_path("Shaders/Compiled");
    file_path /= vs_name;
    FILE* file_pointer = nullptr;
    fopen_s(&file_pointer, file_path.string().c_str(), "rb");
    if (!file_pointer)return false;
    fseek(file_pointer, file_seek_start, SEEK_END);
    long file_size = ftell(file_pointer);
    fseek(file_pointer, file_seek_start, SEEK_SET);
    std::unique_ptr<unsigned char[]> shader_data = std::make_unique<unsigned char[]>(file_size);
    fread(shader_data.get(), file_size, read_count, file_pointer);
    fclose(file_pointer);

    //リフレクションによる解析
    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflector;
    HRESULT hr = D3DReflect(shader_data.get(), file_size, IID_ID3D11ShaderReflection, reinterpret_cast<void**>(reflector.GetAddressOf()));
    if (FAILED(hr))return false;
    D3D11_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);
    std::vector<D3D11_INPUT_ELEMENT_DESC> input_elements;

    //入力レイアウトの自動生成
    for (UINT i = 0; i < shader_desc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);
        D3D11_INPUT_ELEMENT_DESC element_desc = {};
        element_desc.SemanticName = param_desc.SemanticName;
        element_desc.SemanticIndex = param_desc.SemanticIndex;
        element_desc.Format = DetermineDxgiFormat(param_desc.ComponentType, param_desc.Mask);
        element_desc.InputSlot = default_input_slot;
        element_desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        element_desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        element_desc.InstanceDataStepRate = default_instance_data_step_rate;
        input_elements.push_back(element_desc);
    }

    //リソースの生成
    auto device = Graphics::Instance().GetDevice();
    hr = device->CreateInputLayout(input_elements.data(), static_cast<UINT>(input_elements.size()), shader_data.get(), file_size, input_layout.GetAddressOf());
    if (FAILED(hr))return false;

    return true;
}

//====================================================
//シェーダーの変数型とマスク情報からDXGI_FORMATを判定
//====================================================
DXGI_FORMAT CustomShader::DetermineDxgiFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask)
{
    //フォーマットの判定
    if (type == D3D_REGISTER_COMPONENT_FLOAT32)
    {
        if (mask == mask_x)return DXGI_FORMAT_R32_FLOAT;
        if (mask == mask_xy) return DXGI_FORMAT_R32G32_FLOAT;
        if (mask == mask_xyz) return DXGI_FORMAT_R32G32B32_FLOAT;
        if (mask == mask_xyzw) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
    else if (type == D3D_REGISTER_COMPONENT_UINT32)
    {
        if (mask == mask_x) return DXGI_FORMAT_R32_UINT;
        if (mask == mask_xy) return DXGI_FORMAT_R32G32_UINT;
        if (mask == mask_xyz) return DXGI_FORMAT_R32G32B32_UINT;
        if (mask == mask_xyzw) return DXGI_FORMAT_R32G32B32A32_UINT;
    }
    else if (type == D3D_REGISTER_COMPONENT_SINT32)
    {
        if (mask == mask_x) return DXGI_FORMAT_R32_SINT;
        if (mask == mask_xy) return DXGI_FORMAT_R32G32_SINT;
        if (mask == mask_xyz) return DXGI_FORMAT_R32G32B32_SINT;
        if (mask == mask_xyzw) return DXGI_FORMAT_R32G32B32A32_SINT;
    }

    return DXGI_FORMAT_UNKNOWN;
}
