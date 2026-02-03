#include "FbxBone.h"

#include <fbxsdk.h>
#include <unordered_map>

// 行列変換ヘルパー
namespace 
{
    DirectX::XMFLOAT4X4 ToXMFloat4x4(const FbxAMatrix& src) 
    {
        DirectX::XMFLOAT4X4 dest;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                dest.m[r][c] = static_cast<float>(src.Get(r, c));
        return dest;
    }
}

// シーンからボーン情報を抽出
void FbxBone::Fetch(fbxsdk::FbxScene* scene, std::vector<BoneData>& out_bones)
{
    out_bones.clear();
    int node_count = scene->GetSrcObjectCount<FbxNode>();
    std::unordered_map<std::string, int64_t> bone_name_to_index;

    //ノードリストからボーンとして扱えるものを抽出
    for (int i = 0; i < node_count; i++)
    {
        FbxNode* node = scene->GetSrcObject<FbxNode>(i);
        FbxNodeAttribute* attribute = node->GetNodeAttribute();

        if (attribute &&
            (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton ||
                attribute->GetAttributeType() == FbxNodeAttribute::eNull ||
                attribute->GetAttributeType() == FbxNodeAttribute::eMesh))
        {
            BoneData bone;
            bone.name = node->GetName();
            bone.parent_index = -1;

            //バインドポーズの逆行列（初期姿勢）を計算
            FbxAMatrix global = node->EvaluateGlobalTransform();
            bone.offset_transform = ToXMFloat4x4(global.Inverse());

            out_bones.push_back(std::move(bone));
            bone_name_to_index[node->GetName()] = out_bones.size() - 1;
        }
    }

    //親子関係（インデックス）の解決
    for (auto& bone : out_bones)
    {
        // 名前からノードを取得
        FbxNode* node = scene->FindNodeByName(bone.name.c_str());
        if (!node) continue;

        FbxNode* parent = node->GetParent();
        if (parent && bone_name_to_index.count(parent->GetName()) > 0)
        {
            bone.parent_index = bone_name_to_index[parent->GetName()];
        }
        else
        {
            bone.parent_index = -1; // ルートボーン
        }
    }
}