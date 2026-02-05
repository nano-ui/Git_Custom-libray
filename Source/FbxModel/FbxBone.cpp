#include "FbxBone.h"

#include <fbxsdk.h>
#include <unordered_map>
#include <queue>

//=====================
// 行列変換ヘルパー
//=====================
namespace 
{
    //FBXの行列(FbxAMatrix)をDirectXの行列(XMFLOAT4X4)に変換
    DirectX::XMFLOAT4X4 ToXMFloat4x4(const FbxAMatrix& src) 
    {
        DirectX::XMFLOAT4X4 dest;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                dest.m[r][c] = static_cast<float>(src.Get(r, c));
        return dest;
    }

    //スキン情報を持つメッシュノードを再帰的に検索
    FbxMesh* FindSkinnedMesh(FbxNode* node)
    {
        if (!node) return nullptr;

        FbxNodeAttribute* attr = node->GetNodeAttribute();
        if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            FbxMesh* mesh = node->GetMesh();

            //スキンでフォーマを持っているか確認
            if (mesh && mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
            {
                return mesh;
            }
        }

        //子ノードを検索
        for (int i = 0; i < node->GetChildCount(); i++)
        {
            FbxMesh* result = FindSkinnedMesh(node->GetChild(i));
            if (result)return result;
        }
        return nullptr;
    }

}

//============================
// シーンからボーン情報を抽出
//============================
void FbxBone::Fetch(fbxsdk::FbxScene* scene, std::vector<BoneData>& out_bones)
{
    //出力用配列をクリアして初期化
    out_bones.clear();

    //スキンメッシュを持つメッシュを検索
    FbxNode* root_node = scene->GetRootNode();
    FbxMesh* target_mesh = FindSkinnedMesh(root_node);

    if (!target_mesh)
    {
        return;
    }

    //スキンデフォーマーを取得
    FbxSkin* skin = static_cast<FbxSkin*>(target_mesh->GetDeformer(0, FbxDeformer::eSkin));

    if (!skin)
    {
        return;
    }

    //クラスター(ボーン)の数だけループ
    int cluster_count = skin->GetClusterCount();
    out_bones.resize(cluster_count);

    //名前検索用マップ
    std::unordered_map<std::string, int> bone_name_to_index;

    for (int i = 0; i < cluster_count; i++)
    {
        FbxCluster* cluster = skin->GetCluster(i);
    }

}