#include "SkinnedMeshSerializer.h"

#include "../FbxModel/FbxSkinnedResource.h"

#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include "GltfModelSerializer.h"

//DirectX型のシリアライズ定義
namespace DirectX {
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT2& v) { archive(v.x, v.y); }
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT3& v) { archive(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT4& v) { archive(v.x, v.y, v.z, v.w); }
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT4X4& m) {
        archive(m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44);
    }
}

//各構造体のシリアライズ定義
template<class Archive>
void serialize(Archive& archive, MeshSubset& s)
{
    archive(s.material_unique_id, s.material_name, s.start_index_location, s.index_count);
}

template<class Archive>
void serialize(Archive& archive, MeshVertex& v)
{
    archive(v.position, v.normal, v.tangent, v.texcoord, v.bone_weights, v.bone_indices);
}

template<class Archive>
void serialize(Archive& archive, MeshData& m)
{
    archive(m.unique_id, m.name, m.node_index, m.default_global_transform, m.subsets, m.bounding_box, m.vertices, m.indices);
}

template<class Archive>
void serialize(Archive& archive, BoneData& b)
{
    archive(b.name, b.parent_index, b.offset_transform);
}

template<class Archive>
void serialize(Archive& archive, AnimationKeyframeNode& n)
{
    archive(n.global_transform, n.scaling, n.rotation, n.translation);
}

template<class Archive>
void serialize(Archive& archive, AnimationClip& a)
{
    archive(a.name, a.sampling_rate, a.sequence);
}

template<class Archive>
void serialize(Archive& archive, MaterialData& m)
{
    archive(m.unique_id, m.name, m.color, m.texture_filenames);
}

//ファイルから読み込み
bool SkinnedMeshSerializer::Load(const std::string& filename, std::shared_ptr<FbxSkinnedResource> resource)
{
    if (!resource)return false;

    try
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs.is_open()) return false;

        cereal::BinaryInputArchive archive(ifs);

        archive(resource->meshes, resource->bones, resource->animations, resource->materials);

        return true;
    }
    catch (...)
    {
        //ファイル破損などのエラー
        return false;
    }
}

//ファイルへ保存
bool SkinnedMeshSerializer::Save(const std::string& filename, std::shared_ptr<FbxSkinnedResource>& resource)
{
    try
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs.is_open()) return false;

        cereal::BinaryOutputArchive archive(ofs);

        archive(resource->meshes, resource->bones, resource->animations, resource->materials);

        return true;
    }
    catch (...)
    {
        //ファイル破損などのエラー
        return false;
    }
}
