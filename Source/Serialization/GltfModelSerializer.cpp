#include "GltfModelSerializer.h"
#include "../GltfModel/GltfModelData.h"

#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>

//====================================================
//DirectX型のシリアライズ定義
//====================================================
namespace DirectX
{
	template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT4X4& m)
	{
		archive(m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44);
	}
}

//====================================================
//GltfModelData内の各構造体のシリアライズ定義群
//====================================================

//--------------------------------------------------
// シーンとノード
//--------------------------------------------------
template<class Archive> void serialize(Archive& archive, GltfModelData::scene& s)
{
	archive(s.name, s.nodes);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::node& n)
{
	archive(
		n.name,
		n.skin,
		n.mesh,
		n.children,
		n.parent,
		n.has_matrix,
		n.matrix,
		n.rotation,
		n.scale,
		n.translation,
		n.global_transform
	);
}

//--------------------------------------------------
// メッシュとプリミティブ
//--------------------------------------------------
template<class Archive> void serialize(Archive& archive, GltfModelData::buffer_view& b)
{
	archive(b.format, b.buffer, b.stride_in_bytes, b.byte_offset, b.count);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::mesh::primitive& p)
{
	archive(p.material, p.vertex_buffer_views, p.index_buffer_view);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::mesh& m)
{
	archive(m.name, m.primitives);
}

//--------------------------------------------------
// マテリアル関連
//--------------------------------------------------
template<class Archive> void serialize(Archive& archive, GltfModelData::texture_info& t) { archive(t.index, t.texcoord); }
template<class Archive> void serialize(Archive& archive, GltfModelData::normal_texture_info& t) { archive(t.index, t.texcoord, t.scale); }
template<class Archive> void serialize(Archive& archive, GltfModelData::occlusion_texture_info& t) { archive(t.index, t.texcoord, t.strength); }
template<class Archive> void serialize(Archive& archive, GltfModelData::pbr_metallic_roughness& p)
{
	archive(p.basecolor_factor, p.basecolor_texture, p.metallic_factor, p.roughness_factor, p.metallic_roughness_texture);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::material::cbuffer& c)
{
	archive(c.emissive_factor, c.alpha_mode, c.alpha_cutoff, c.double_sided, c.pbr_metallic_roughness, c.normal_texture, c.occlusion_texture, c.emissive_texture);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::material& m)
{
	archive(m.name, m.data);
}

//--------------------------------------------------
// テクスチャと画像（ここで画像生データも保存される）
//--------------------------------------------------
template<class Archive> void serialize(Archive& archive, GltfModelData::texture& t)
{
	archive(t.name, t.source);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::image& i)
{
	archive(i.name, i.width, i.height, i.component, i.bits, i.pixel_type, i.buffer_view, i.mime_type, i.uri, i.as_is, i.raw_image_data);
}

//--------------------------------------------------
// スキンとアニメーション
//--------------------------------------------------
template<class Archive> void serialize(Archive& archive, GltfModelData::skin& s)
{
	// ※skin構造体にnameは存在しないので除外してあります
	archive(s.inverse_bind_matrices, s.joints);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::animation::channel& c)
{
	archive(c.sampler, c.target_node, c.target_path);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::animation::sampler& s)
{
	archive(s.input, s.output, s.interpolation);
}
template<class Archive> void serialize(Archive& archive, GltfModelData::animation& a)
{
	archive(a.name, a.duration, a.channels, a.samplers, a.timelines, a.scales, a.rotations, a.translations);
}
//---------------------------------------
//バイナリファイルからデータを読み込み
//---------------------------------------
bool GltfModelSerializer::Load(const std::string& filename, const std::shared_ptr<GltfModelData>& resource)
{
	if (!resource) return false;

	try
	{
		//--------------------------------------------------
		// バイナリ読み込みモードでストリームを開く
		//--------------------------------------------------
		std::ifstream input_file(filename, std::ios::binary);	// 指定されたファイルをバイナリ形式で開く
		if (!input_file.is_open()) return false;				// ファイルが見つからなかった場合は失敗を返す

		cereal::BinaryInputArchive archive(input_file);			// cerealのバイナリ入力アーカイブを生成する

		//--------------------------------------------------
		// 全ての生データをファイルから一括で復元する
		//--------------------------------------------------
		archive(
			resource->filename,
			resource->default_scene,
			resource->raw_buffers,
			resource->scenes,
			resource->nodes,
			resource->meshes,
			resource->materials,
			resource->textures,
			resource->images,
			resource->skins,
			resource->animations,
			resource->animation_index_map
		);

		return true;	// 読み込み成功としてtrueを返す
	}
	catch (...)			// 読み込み中に予期せぬ例外（ファイル破損など）が発生した場合
	{
		return false;	// 失敗としてfalseを返す
	}
}

//===================================
//データをバイナリファイルへ保存
//===================================
bool GltfModelSerializer::Save(const std::string& filename, const std::shared_ptr<GltfModelData>& resource)
{
	if (!resource) return false;																	

	try
	{
		//--------------------------------------------------
		// バイナリ書き込みモードでストリームを開く
		//--------------------------------------------------
		std::ofstream output_file(filename, std::ios::binary);	//指定されたファイルをバイナリ書き込み形式で開く（上書き）
		if (!output_file.is_open()) return false;				//ファイルが作成できなかった場合は失敗を返す

		cereal::BinaryOutputArchive archive(output_file);		// erealのバイナリ出力アーカイブを生成する

		//--------------------------------------------------
		// 全ての生データをファイルへ一括で書き込む
		//--------------------------------------------------
		archive(
			resource->filename,
			resource->default_scene,
			resource->raw_buffers,
			resource->scenes,
			resource->nodes,
			resource->meshes,
			resource->materials,
			resource->textures,
			resource->images,
			resource->skins,
			resource->animations,
			resource->animation_index_map
		);

		return true;	//書き込み成功としてtrueを返す
	}
	catch (...)			//書き込み中に予期せぬ例外が発生した場合
	{
		return false;	//失敗としてfalseを返す
	}
}
