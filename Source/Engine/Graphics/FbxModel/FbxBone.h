#pragma once
#include <vector>
#include <memory>
#include "../Engine/Graphics/FbxModel/FbxModelData.h"

namespace fbxsdk 
{
    class FbxScene;
}

class FbxBone
{
public:
    // シーンからボーン情報を抽出
    static void Fetch(fbxsdk::FbxScene* scene, std::vector<BoneData>& out_bones);
};