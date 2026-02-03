#pragma once
#include <unordered_map>
#include <string>
#include <d3d11.h>

#include "../Common/ModelData.h"

namespace fbxsdk {
    class FbxScene;
}

class FbxMaterial
{
public:
    //マテリアル情報とテクスチャ読み込み
    static void Fetch(
        ID3D11Device* device,
        fbxsdk::FbxScene* scene,
        const std::string& fbx_filename,
        std::unordered_map<uint64_t, MaterialData>& out_materials);
};