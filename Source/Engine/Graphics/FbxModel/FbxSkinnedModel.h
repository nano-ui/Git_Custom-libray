#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

#include "../FbxModel/FbxSkinnedResource.h"


//シーン定数バッファ用
struct SceneConstants
{
    DirectX::XMFLOAT4X4 view_projection;
    DirectX::XMFLOAT4 light_direction;
    DirectX::XMFLOAT4 camera_position;
};


class FbxSkinnedModel
{
public:
    //コンストラクタ
    FbxSkinnedModel(std::shared_ptr<FbxSkinnedResource> resource);
   ~FbxSkinnedModel() = default;

   //更新処理
   void AnimationUpdate(float delta_time);

   //描画処理
   void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& color);

   //アニメーション更新処理
   void PlayAnimation(const std::string& clip_name, bool loop = true);

private:
    std::shared_ptr<FbxSkinnedResource> resource;

    //アニメーション制御
    std::string current_clip_name;  //現在のアニメーション名
    float current_time = 0.0f;      //現在のアニメーション時間
    bool is_loop = true;            //ループ再生用のフラグ

    std::vector<DirectX::XMFLOAT4X4> global_transforms;//計算用の一時的なグローバル行列
    std::vector<DirectX::XMFLOAT4X4> bone_transforms;//計算済みのボーン行列

    //定数バッファ構造体
    struct Constants
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4 material_color;
        DirectX::XMFLOAT4X4 bone_transforms[256];
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

    //描画に使うシェーダープログラム
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
};

