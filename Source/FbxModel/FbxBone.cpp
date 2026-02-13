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

    //クラスターから初期姿勢の逆行列（Inverse Bind Pose）を計算
    DirectX::XMFLOAT4X4 CalculateOffsetMatrix(FbxCluster* cluster)
    {
        if (!cluster)
        {
            //クラスターがない場合は単位行列を返す
            DirectX::XMFLOAT4X4 identity;
            DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
            return identity;
        }

        FbxAMatrix transform_matrix;
        FbxAMatrix transform_link_matrix;
        FbxAMatrix global_bindpose_inverse_matrix;

        //モデル（メッシュ）の変換行列
        cluster->GetTransformMatrix(transform_matrix);
        //ボーンの変換行列
        cluster->GetTransformLinkMatrix(transform_link_matrix);

        //逆行列の計算: InverseBindPose = TransformLink^{-1} * Transform
        global_bindpose_inverse_matrix = transform_link_matrix.Inverse() * transform_matrix;

        return ToXMFloat4x4(global_bindpose_inverse_matrix);
    }

    //再帰的にノードを探索し、階層順にリストに追加
    void TraverseHierachy(
        FbxNode* node,
        int parent_index,
        std::vector<BoneData>& out_bones,
        const std::unordered_map<FbxNode*, FbxCluster*>& bone_cluster_map
    )
    {
        //このノードが「有効なボーン（スキニングに使われている）」かどうか判定
        bool is_valid_bone = (bone_cluster_map.find(node) != bone_cluster_map.end());

        if (is_valid_bone)
        {
            BoneData bone;
            bone.name = node->GetName();
            bone.parent_index = parent_index;
            bone.unique_id = node->GetUniqueID();
            bone.node_index = -1;

            //マップから対応するクラスターを取得して、オフセット行列を計算
            FbxCluster* cluster = bone_cluster_map.at(node);
            bone.offset_transform = CalculateOffsetMatrix(cluster);

            //リストに追加
            out_bones.push_back(bone);

            //親インデックスを更新（今追加したボーンが、次の子の親になる）
            parent_index = static_cast<int>(out_bones.size() - 1);
        }

        //子ノードを再帰的に処理
        for (int i = 0; i < node->GetChildCount(); i++)
        {
            TraverseHierachy(node->GetChild(i), parent_index, out_bones, bone_cluster_map);
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

    //ボーンのノード, Value: そのボーンを管理しているクラスター
    std::unordered_map<FbxNode*, FbxCluster*> bone_cluster_map;

    //スキンメッシュを持つメッシュを検索
    std::set<FbxNode*> valid_bones;

    int geometry_count = scene->GetGeometryCount();
    for (int i = 0; i < geometry_count; i++)
    {
        FbxGeometry* geo = scene->GetGeometry(i);
        if (geo->GetAttributeType() == FbxNodeAttribute::eMesh)continue;

        FbxMesh* mesh = static_cast<FbxMesh*>(geo);

		int skin_count = mesh->GetDeformerCount(FbxDeformer::eSkin);
        for (int j = 0; j < skin_count; j++)
        {
            if (mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
            {
                FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
                if (!skin)continue;
                int cluster_count = skin->GetClusterCount();
                for (int k = 0; i < cluster_count; k++)
                {
                    FbxCluster* cluster = skin->GetCluster(k);
                    if (cluster) continue;
                    FbxNode* bone_node = cluster->GetLink();
                    if (bone_node)
                    {
                        bone_cluster_map[bone_node] = cluster;
                    }
                }
            }
        }
    }

	FbxNode* root_node = scene->GetRootNode();
    if (root_node)
    {
        //ルートから再帰的に探索してリストを作成
        TraverseHierachy(root_node, -1, out_bones, bone_cluster_map);
    }
}