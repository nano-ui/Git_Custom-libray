#include "FbxBone.h"

#include <fbxsdk.h>
#include <unordered_map>

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
}

//============================
// シーンからボーン情報を抽出
//============================
void FbxBone::Fetch(fbxsdk::FbxScene* scene, std::vector<BoneData>& out_bones)
{
    //出力用配列をクリアして初期化
    out_bones.clear();

    //シーン内の全ノード数を取得
    int node_count = scene->GetSrcObjectCount<FbxNode>();

    //ボーン名からインデックスを逆引きするためのマップ
    std::unordered_map<std::string, int64_t> bone_name_to_index;

    //---------------------------------------------
    //ノードリストからボーンとして扱えるものを抽出
    //---------------------------------------------
    for (int i = 0; i < node_count; i++)
    {
        //1番目のノードを取得
        FbxNode* node = scene->GetSrcObject<FbxNode>(i);

        //ノードの属性を取得
        FbxNodeAttribute* attribute = node->GetNodeAttribute();

        //属性が存在し、かつボーンとして扱うべきタイプか判定
        if (attribute &&
            (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton ||
                attribute->GetAttributeType() == FbxNodeAttribute::eNull ||
                attribute->GetAttributeType() == FbxNodeAttribute::eMesh))
        {
            //ボーンデータを作成
            BoneData bone;

            //ノード名をボーン名として保存
            bone.name = node->GetName();

            //親インデックスは後で解放するため、一旦-1に設定
            bone.parent_index = -1;

            //グローバル変換行列を取得
            FbxAMatrix global = node->EvaluateGlobalTransform();

            //逆行列を計算してオフセット行列とする
            bone.offset_transform = ToXMFloat4x4(global.Inverse());

            //リストに追加
            out_bones.push_back(std::move(bone));

            //名前と「追加した場所のインデックス」をマップに記録
            bone_name_to_index[node->GetName()] = out_bones.size() - 1;
        }
    }

    //-------------------------------
    //親子関係（インデックス）の解決
    //-------------------------------
    //登録された全ボーンについてループ
    for (auto& bone : out_bones)
    {
        // ボーン名を使ってFBXのノードオブジェクトを再検索
        FbxNode* node = scene->FindNodeByName(bone.name.c_str());

        //ノードが見つからなけらばスキップ
        if (!node) continue;

        //現在のノードの親ノードを取得
        FbxNode* parent = node->GetParent();

        //親が存在し、かつその親も「ボーンリスト」に含まれているか確認
        if (parent && bone_name_to_index.count(parent->GetName()) > 0)
        {
            //マップを使って親のインデックスを取得して設定
            bone.parent_index = bone_name_to_index[parent->GetName()];
        }
        else
        {
            //親がいない、または親がボーンとして管理されていない場合はルート扱い
            bone.parent_index = -1;
        }
    }
}