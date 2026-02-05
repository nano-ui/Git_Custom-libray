#include "FbxBone.h"

#include <fbxsdk.h>
#include <unordered_map>
#include <queue>
#include <set>

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

    //再帰的にノードを探索し、階層順にリストに追加
    void TraverseHierachy(
        FbxNode* node,
        int parent_index,
        std::vector<BoneData>& out_bones,
        FbxSkin* skin,
        const std::set<FbxNode*>& valid_bones
    )
    {
        bool is_valid = false;
        if (valid_bones.count(node) > 0)
        {
            is_valid = true;
        }
        else
        {
            //ノードの属性を取得
            FbxNodeAttribute* attr = node->GetNodeAttribute();
            if (attr)
            {
                auto type = attr->GetAttributeType();

                //骨、または階層構造用のヌルノードであれば、階層をつなぐために登録
                if (type == FbxNodeAttribute::eSkeleton || type == FbxNodeAttribute::eNull)
                {
                    is_valid = true;
                }
                else if (parent_index != -1)
                {
                    is_valid = true;
                }
            }
        }

        int current_index = -1;

        if (is_valid)
        {
            BoneData bone;
            bone.name = node->GetName();
            bone.parent_index = parent_index;
            bone.unique_id = node->GetUniqueID();
            bone.node_index = -1;

            //オフセット行列の計算
            FbxAMatrix offset_matrix;
            offset_matrix.SetIdentity();

            if (skin)
            {
                int cluster_count = skin->GetClusterCount();
                for (int i = 0; i < cluster_count; i++)
                {
                    FbxCluster* cluster = skin->GetCluster(i);
                    if (cluster->GetLink() == node)
                    {
                        FbxAMatrix reference_global, cluster_global;
                        cluster->GetTransformMatrix(reference_global);
                        cluster->GetTransformLinkMatrix(cluster_global);

                        FbxNode* mesh_node = skin->GetGeometry()->GetNode();
                        FbxAMatrix geometry_transform;
                        if (mesh_node)
                        {
                            const FbxVector4 T = mesh_node->GetGeometricTranslation(FbxNode::eSourcePivot);
                            const FbxVector4 R = mesh_node->GetGeometricRotation(FbxNode::eSourcePivot);
                            const FbxVector4 S = mesh_node->GetGeometricScaling(FbxNode::eSourcePivot);
                            geometry_transform.SetT(T);
                            geometry_transform.SetR(R);
                            geometry_transform.SetS(S);
                        }
                        else
                        {
                            geometry_transform.SetIdentity();
                        }

                        reference_global = reference_global * geometry_transform;
                        FbxAMatrix offset = cluster_global.Inverse() * reference_global;
                        bone.offset_transform = ToXMFloat4x4(offset);

                        break;
                    }
                }
            }

            //リストに追加
            out_bones.push_back(bone);
            current_index = static_cast<int>(out_bones.size() - 1);
        }

        //子ノードを再帰検索
        int next_parent_index = (current_index != -1) ? current_index : parent_index;

        //全ての子ノードを走査
        for (int i = 0; i < node->GetChildCount(); i++)
        {
            TraverseHierachy(node->GetChild(i), next_parent_index, out_bones, skin, valid_bones);
        }
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
    FbxSkin* skin = nullptr;

    int geometry_count = scene->GetGeometryCount();
    for (int i = 0; i < geometry_count; i++)
    {
        FbxGeometry* geo = scene->GetGeometry(i);
        if (geo->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            FbxMesh* mesh = static_cast<FbxMesh*>(geo);
            if (mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
            {
                skin = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
                break;
            }
        }
    }

    //有効なボーン(クラスター)の集合を作成
    std::set<FbxNode*> valid_bones;
    if (skin)
    {
        int cluster_count = skin->GetClusterCount();
        for (int i = 0; i < cluster_count; i++)
        {
            FbxNode* link = skin->GetCluster(i)->GetLink();
            if (link)
            {
                valid_bones.insert(link);
            }
        }
    }

    //ルートから再帰的に探索してリストを作成
    TraverseHierachy(root_node, -1, out_bones, skin, valid_bones);
}