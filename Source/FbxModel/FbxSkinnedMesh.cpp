#include "FbxAnimation.h"

#include <fbxsdk.h>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <vector>
#include "FbxSkinnedMesh.h"
#include "../FbxModel/FbxMaterial.h"

//==============
//変換ヘルパー
//==============
namespace {
	DirectX::XMFLOAT4X4 ToXMFloat4x4(const FbxAMatrix& src) {
		DirectX::XMFLOAT4X4 dest;
		for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) dest.m[r][c] = (float)src.Get(r, c);
		return dest;
	}
	DirectX::XMFLOAT3 ToXMFloat3(const FbxDouble3& src) {
		return DirectX::XMFLOAT3((float)src[0], (float)src[1], (float)src[2]);
	}
	DirectX::XMFLOAT2 ToXMFloat2(const FbxDouble2& src) {
		return DirectX::XMFLOAT2((float)src[0], (float)src[1]);
	}
	DirectX::XMFLOAT3 TransformVector(const FbxDouble3& src, const FbxAMatrix& matrix) {
		FbxVector4 vec(src[0], src[1], src[2], 0.0);
		vec = matrix.MultT(vec);
		return DirectX::XMFLOAT3((float)vec[0], (float)vec[1], (float)vec[2]);
	}
	DirectX::XMFLOAT3 TransformPoint(const FbxDouble3& src, const FbxAMatrix& matrix) {
		FbxVector4 vec(src[0], src[1], src[2], 1.0);
		vec = matrix.MultT(vec);
		return DirectX::XMFLOAT3((float)vec[0], (float)vec[1], (float)vec[2]);
	}
}

//=======================================
// ウェイト情報の取得
//=======================================
void FbxSkinnedMesh::FetchBoneInfuences(
	const fbxsdk::FbxMesh* fbx_mesh,
	const std::vector<BoneData>& bones,
	std::vector<BoneInfluencePerControlPoint>& influences)
{
	int control_points_count = fbx_mesh->GetControlPointsCount();
	influences.resize(control_points_count);

	int skin_count = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int i = 0; i < skin_count; i++)
	{
		FbxSkin* skin = static_cast<FbxSkin*>(fbx_mesh->GetDeformer(i, FbxDeformer::eSkin));
		int cluster_count = skin->GetClusterCount();

		for (int j = 0; j < cluster_count; j++)
		{
			FbxCluster* cluster = skin->GetCluster(j);
			FbxNode* link_node = cluster->GetLink();
			if (!link_node) continue;

			uint64_t bone_unique_id = link_node->GetUniqueID();
			int64_t bone_index = -1;

			// ボーンインデックスの検索
			for (size_t k = 0; k < bones.size(); ++k)
			{
				if (bones[k].unique_id == bone_unique_id)
				{
					bone_index = static_cast<int64_t>(k);
					break;
				}
			}

			if (bone_index == -1) continue;

			int index_count = cluster->GetControlPointIndicesCount();
			int* indices = cluster->GetControlPointIndices();
			double* weights = cluster->GetControlPointWeights();

			for (int k = 0; k < index_count; k++)
			{
				int control_point_index = indices[k];
				float weight = static_cast<float>(weights[k]);

				BoneInfluence influence;
				influence.bone_index = static_cast<uint32_t>(bone_index);
				influence.weight = weight;

				influences[control_point_index].push_back(influence);
			}
		}
	}
}

//=============================================================================
// メッシュデータの抽出
//=============================================================================
void FbxSkinnedMesh::Fetch(
	ID3D11Device* device,
	fbxsdk::FbxScene* scene,
	const std::string& fbx_filename,
	const std::vector<BoneData>& bones,
	std::vector<MeshData>& out_meshes,
	std::unordered_map<uint64_t, MaterialData>& out_materials)
{
	out_meshes.clear();

	// マテリアル情報の抽出
	FbxMaterial::Fetch(device, scene, fbx_filename, out_materials);

	// メッシュの抽出
	int geometry_count = scene->GetGeometryCount();
	for (int i = 0; i < geometry_count; i++)
	{
		FbxGeometry* geo = scene->GetGeometry(i);
		if (geo->GetAttributeType() != FbxNodeAttribute::eMesh) continue;

		FbxMesh* fbx_mesh = static_cast<FbxMesh*>(geo);
		FbxNode* node = fbx_mesh->GetNode();

		MeshData mesh_data;
		mesh_data.unique_id = node->GetUniqueID();
		mesh_data.name = node->GetName();
		mesh_data.node_index = 0;

		// 接線ベクトルの生成
		fbx_mesh->GenerateTangentsData(0, false);

		// ウェイト情報の取得
		std::vector<BoneInfluencePerControlPoint> influences;
		FetchBoneInfuences(fbx_mesh, bones, influences);

		//-------------------------------------------------------------------------
		// サブセット（マテリアル）の準備
		// unordered_mapを使わず、vectorを使ってFBXのマテリアル順序を維持する
		//-------------------------------------------------------------------------
		int material_count = node->GetMaterialCount();
		mesh_data.subsets.resize(material_count > 0 ? material_count : 1);

		for (int m = 0; m < material_count; ++m)
		{
			const FbxSurfaceMaterial* fbx_mat = node->GetMaterial(m);
			mesh_data.subsets[m].material_unique_id = fbx_mat->GetUniqueID();
			mesh_data.subsets[m].material_name = fbx_mat->GetName();
			mesh_data.subsets[m].index_count = 0; // カウンタ初期化
		}

		//-------------------------------------------------------------------------
		// [PASS 1] ポリゴンを走査して、各サブセットのインデックス数をカウント
		//-------------------------------------------------------------------------
		int polygon_count = fbx_mesh->GetPolygonCount();
		FbxGeometryElementMaterial* material_element = fbx_mesh->GetElementMaterial(0);

		for (int p = 0; p < polygon_count; p++)
		{
			int material_index = 0;
			if (material_count > 0 && material_element)
			{
				switch (material_element->GetMappingMode())
				{
				case FbxGeometryElement::eAllSame:
					material_index = material_element->GetIndexArray().GetAt(0);
					break;
				case FbxGeometryElement::eByPolygon:
					material_index = material_element->GetIndexArray().GetAt(p);
					break;
				}
			}
			// 安全策
			if (material_index < 0 || material_index >= material_count) material_index = 0;

			// 三角形リストとして扱うため、1ポリゴンにつき3インデックス増加
			mesh_data.subsets[material_index].index_count += 3;
		}

		//-------------------------------------------------------------------------
		// オフセット計算 (各サブセットの開始位置を確定)
		//-------------------------------------------------------------------------
		uint32_t offset = 0;
		for (auto& subset : mesh_data.subsets)
		{
			subset.start_index_location = offset;
			offset += subset.index_count;
			subset.index_count = 0; // 次のPASS 2でカウンタとして使うためリセット
		}

		//-------------------------------------------------------------------------
		// バッファメモリ確保
		//-------------------------------------------------------------------------
		size_t total_vertex_count = static_cast<size_t>(polygon_count) * 3;
		mesh_data.vertices.resize(total_vertex_count);
		mesh_data.indices.resize(total_vertex_count);

		//-------------------------------------------------------------------------
		// [PASS 2] 頂点データの取得と格納
		//-------------------------------------------------------------------------
		FbxVector4* control_points = fbx_mesh->GetControlPoints();
		FbxGeometryElementNormal* normal_element = fbx_mesh->GetElementNormal(0);
		FbxGeometryElementUV* uv_element = fbx_mesh->GetElementUV(0);
		FbxGeometryElementTangent* tangent_element = fbx_mesh->GetElementTangent(0);
		FbxStringList uv_names;
		fbx_mesh->GetUVSetNames(uv_names);

		for (int p = 0; p < polygon_count; p++)
		{
			// マテリアルインデックスの再取得
			int material_index = 0;
			if (material_count > 0 && material_element)
			{
				switch (material_element->GetMappingMode())
				{
				case FbxGeometryElement::eAllSame: material_index = material_element->GetIndexArray().GetAt(0); break;
				case FbxGeometryElement::eByPolygon: material_index = material_element->GetIndexArray().GetAt(p); break;
				}
			}
			if (material_index < 0 || material_index >= material_count) material_index = 0;

			// 書き込み先の計算
			MeshSubset& subset = mesh_data.subsets[material_index];
			uint32_t write_offset = static_cast<uint32_t>(subset.start_index_location) + subset.index_count;

			for (int v = 0; v < 3; v++)
			{
				int vertex_index = p * 3 + v;                // 今回作成する頂点の配列インデックス
				int ctrl_idx = fbx_mesh->GetPolygonVertex(p, v); // FBXのコントロールポイントインデックス

				MeshVertex vertex;

				// 座標
				vertex.position = ToXMFloat3(control_points[ctrl_idx]);

				// 法線
				if (normal_element)
				{
					FbxVector4 normal;
					fbx_mesh->GetPolygonVertexNormal(p, v, normal);
					vertex.normal = DirectX::XMFLOAT3((float)normal[0], (float)normal[1], (float)normal[2]);
				}

				// UV
				if (uv_element && uv_names.GetCount() > 0)
				{
					FbxVector2 uv;
					bool unmapped;
					fbx_mesh->GetPolygonVertexUV(p, v, uv_names[0], uv, unmapped);
					vertex.texcoord.x = (float)uv[0];
					vertex.texcoord.y = 1.0f - (float)uv[1]; // V反転
				}

				// 接線 (Tangent)
				if (tangent_element)
				{
					int tangent_index = 0;
					if (tangent_element->GetMappingMode() == FbxGeometryElement::eByControlPoint)
						tangent_index = (tangent_element->GetReferenceMode() == FbxGeometryElement::eDirect) ? ctrl_idx : tangent_element->GetIndexArray().GetAt(ctrl_idx);
					else
						tangent_index = (tangent_element->GetReferenceMode() == FbxGeometryElement::eDirect) ? vertex_index : tangent_element->GetIndexArray().GetAt(vertex_index);

					FbxVector4 t = tangent_element->GetDirectArray().GetAt(tangent_index);
					vertex.tangent = DirectX::XMFLOAT4((float)t[0], (float)t[1], (float)t[2], (float)t[3]);
				}

				// ウェイト
				if (ctrl_idx < (int)influences.size())
				{
					const auto& inf_list = influences[ctrl_idx];
					float total_weight = 0;
					for (size_t w = 0; w < inf_list.size() && w < 4; ++w)
					{
						vertex.bone_indices[w] = inf_list[w].bone_index;
						vertex.bone_weights[w] = inf_list[w].weight;
						total_weight += inf_list[w].weight;
					}
					// 正規化
					if (total_weight > 0) {
						for (int w = 0; w < 4; ++w) vertex.bone_weights[w] /= total_weight;
					}
				}

				// データの格納
				mesh_data.vertices[vertex_index] = std::move(vertex);
				mesh_data.indices[write_offset + v] = vertex_index;
			}

			// カウンタを進める
			subset.index_count += 3;
		}

		// GPUバッファ作成
		CreateComBuffers(device, mesh_data);

		out_meshes.push_back(mesh_data);
	}
}

//=================
//GPUバッファ作成
//=================
void FbxSkinnedMesh::CreateComBuffers(
	ID3D11Device* device,
	MeshData& mesh)
{
	D3D11_BUFFER_DESC buffer_desc{};
	D3D11_SUBRESOURCE_DATA subresource_data{};

	//-------------------
	//頂点バッファの作成
	//-------------------
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(MeshVertex) * mesh.vertices.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	subresource_data.pSysMem = mesh.vertices.data();

	device->CreateBuffer(&buffer_desc, &subresource_data, mesh.vertex_buffer.GetAddressOf());

	//-----------------------
	//インデックスバッファ作成
	//-----------------------
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	subresource_data.pSysMem = mesh.indices.data();

	device->CreateBuffer(&buffer_desc, &subresource_data, mesh.index_buffer.GetAddressOf());
}