#include "SkinnedMeshComponents.h"
#include <fbxsdk.h>
#include <algorithm>
#include <map>
#include <filesystem>
#include <string>
#include "../Graphics/shader.h"
#include "../ObjectsRender/texture.h"
#include "../Serialization/SkinnedMeshSerializer.h"

namespace {
	//行列の変換
	DirectX::XMFLOAT4X4 ToXMFloat4x4(const FbxAMatrix& src)
	{
		DirectX::XMFLOAT4X4 dest;
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				dest.m[r][c] = static_cast<float>(src.Get(r, c));
			}
		}
		return dest;
	}

	//3次元ベクトルの変換
	DirectX::XMFLOAT3 ToXMFloat3(const FbxDouble3& src)
	{
		return DirectX::XMFLOAT3(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]));
	}

	//4次元ベクトル/クォータニオンの変換
	DirectX::XMFLOAT4 ToXMFloat4(const FbxDouble4& src)
	{
		return DirectX::XMFLOAT4(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]), static_cast<float>(src[3]));
	}

	const int MAX_BONE_INFLUENCES = 4;	//最大ボーン影響数
}

//ファイル名を指定してモデルの読み込みを開始
SkinnedMeshResource::SkinnedMeshResource(ID3D11Device* device, const std::string& filename)
{
	LoadFbx(device, filename);

	//GPUバッファの生成
	for (auto& mesh : meshes)
	{
		CreateComBuffers(device, mesh);

		mesh.vertices.clear();
		mesh.vertices.shrink_to_fit();
		mesh.indices.clear();
		mesh.indices.shrink_to_fit();
	}

	for (auto& pair : materials)
	{
		for (size_t tex_idx = 0; tex_idx < 2; tex_idx++)
		{
			MaterialData& material_data = pair.second;
			//ファイル名が取得できている場合
			if (material_data.texture_filenames[tex_idx].length() > 0)
			{
				//FBXファイルのパスを基準に、テクスチャの相対パスを解決
				std::filesystem::path fbx_path(filename);
				std::filesystem::path tex_path = fbx_path.parent_path() / material_data.texture_filenames[tex_idx];

				//テクスチャをファイルから読み込んでSRVを作成
				D3D11_TEXTURE2D_DESC texture2d_desc{};
				load_texture_from_file(
					device,
					tex_path.c_str(),
					material_data.shader_resource_views[tex_idx].GetAddressOf(),
					&texture2d_desc
				);
			}
			else
			{
				//テクスチャがない場合はダミーテクスチャを作成
				uint32_t dummy_color = (tex_idx == 1) ? 0xFFFF7F7F : 0xFFFFFFFF;
				make_dummy_texture(
					device,
					material_data.shader_resource_views[tex_idx].GetAddressOf(),
					dummy_color,
					16
				);
			}
		}
	}


	//-------------------
	//定数バッファの作成
	//-------------------
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(Constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//---------------------
	//シェーダーの読み込み
	//---------------------

	//インプットレイアウトの定義
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		// 名前      インデックス  フォーマット                  スロット  オフセット                    クラス                       インスタンスステップ
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 位置
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 法線
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 接線
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // UV座標
		{ "WEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // ボーンウェイト
		{ "BONES",    0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // ボーンインデックス(Uint)
	};

	//頂点シェーダーとインプットレイアウトの作成
	create_vs_from_cso(
		device,
		"skinned_mesh_vs.cso",
		vertex_shader.ReleaseAndGetAddressOf(),
		input_layout.ReleaseAndGetAddressOf(),
		input_element_desc,
		ARRAYSIZE(input_element_desc)
	);

	//ピクセルシェーダーの作成
	create_ps_from_cso(
		device,
		"skinned_mesh_ps.cso",
		pixel_shader.ReleaseAndGetAddressOf()
	);
}

//別のFBXファイルからアニメーションのみを追加で読み込む
bool SkinnedMeshResource::AppendAnimations(
	const std::string& animation_filenama,
	float sampling_rate)
{
	//------------------------
	//FBX SDKのマネージャ作成
	//------------------------
	FbxManager* manager = FbxManager::Create();
	FbxScene* scene = FbxScene::Create(manager, "");
	FbxImporter* importer = FbxImporter::Create(manager, "");

	//-------------------
	//ファイルの読み込み
	//-------------------
	if (!importer->Initialize(animation_filenama.c_str()) || !importer->Import(scene))
	{
		importer->Destroy();
		manager->Destroy();
		return false;
	}
	importer->Destroy();

	//-------------
	//座標系の変換
	//-------------
	FbxAxisSystem::DirectX.ConvertScene(scene);

	//---------------------------
	//アニメーションデータの抽出
	//---------------------------
	FetchAnimations(scene, sampling_rate);

	manager->Destroy();

	return true;
}

//マテリアルデータの取得
const MaterialData* SkinnedMeshResource::GetMaterial(uint64_t unique_id) const
{
	auto it = materials.find(unique_id);
	return (it != materials.end()) ? &it->second : nullptr;
}

//FBXファイルを読み込み、メモリ上のデータに変換
void SkinnedMeshResource::LoadFbx(ID3D11Device* device, const std::string& filename)
{
	//-----------------
	//FBX SDKの初期化
	//-----------------
	FbxManager* manager = FbxManager::Create();
	FbxScene* scene = FbxScene::Create(manager, "");
	FbxImporter* importer = FbxImporter::Create(manager, "");

	//-----------------
	//ファイル読み込み
	//-----------------
	if (!importer->Initialize(filename.c_str()) || !importer->Import(scene))
	{
		manager->Destroy();
		throw std::runtime_error("Failed to load FBX file.");
	}
	importer->Destroy();

	//-------------
	//座標系の変換
	//-------------
	FbxAxisSystem::DirectX.ConvertScene(scene);

	//-------------------
	//ポリゴンの三角形化
	//-------------------
	FbxGeometryConverter converter(manager);
	converter.Triangulate(scene, true);

	//-------------
	//データの抽出
	//-------------

	//マテリアルの抽出
	FetchMaterials(device, scene, filename);

	//スケルトン（ボーン）の抽出
	FetchSkeleton(scene);

	//メッシュデータの抽出
	FetchMeshes(device, scene);

	//アニメーションの抽出
	FetchAnimations(scene);

	//-----------------
	//FbxManagerの破棄
	//-----------------
	manager->Destroy();
}

//マテリアルデータの抽出
void SkinnedMeshResource::FetchMaterials(ID3D11Device* device, fbxsdk::FbxScene* scene, const std::string& fbx_filename)
{
	int material_count = scene->GetMaterialCount();
	for (int i = 0; i < material_count; i++)
	{
		FbxSurfaceMaterial* fbx_material = scene->GetMaterial(i);
		MaterialData material_data;
		material_data.name = fbx_material->GetName();
		material_data.unique_id = fbx_material->GetUniqueID();

		//----------------------------------------------------------------
		//拡散反射光（Diffuse）プロパティの取得
		//LambertやPhongなどのシェーディングモデルに関わらず色情報を取得
		//----------------------------------------------------------------
		FbxProperty prop = fbx_material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (prop.IsValid())
		{
			FbxDouble3 color = prop.Get<FbxDouble3>();
			material_data.color = DirectX::XMFLOAT4(
				static_cast<float>(color[0]),
				static_cast<float>(color[1]),
				static_cast<float>(color[2]),
				1.0f
			);

			//-----------------
			//テクスチャの取得
			//-----------------
			int texture_count = prop.GetSrcObjectCount<FbxFileTexture>();
			if (texture_count > 0)
			{
				FbxFileTexture* texture = prop.GetSrcObject<FbxFileTexture>(0);
				if (texture)
				{
					//---------------------------
					//テクスチャファイル名を保存
					//---------------------------
					material_data.texture_filenames[0] = texture->GetRelativeFileName();
				}
			}
		}

		//-----------------
		//法線マップの取得
		//-----------------
		prop = fbx_material->FindProperty(FbxSurfaceMaterial::sNormalMap);
		if (prop.IsValid())
		{
			int texture_count = prop.GetSrcObjectCount<FbxFileTexture>();
			if (texture_count > 0)
			{
				FbxFileTexture* texture = prop.GetSrcObject<FbxFileTexture>();
				if (texture)
				{
					material_data.texture_filenames[1] = texture->GetRelativeFileName();
				}
			}
		}

		//---------------------
		//テクスチャの読み込み
		//---------------------
		for (size_t tex_idx = 0; tex_idx < 2; tex_idx++)
		{
			//ファイル名が取得できている場合
			if (material_data.texture_filenames[tex_idx].length() > 0)
			{
				//FBXファイルのパスを基準に、テクスチャの相対パスを解決
				std::filesystem::path fbx_path(fbx_filename);
				std::filesystem::path tex_path = fbx_path.parent_path() / material_data.texture_filenames[tex_idx];
				
				//テクスチャをファイルから読み込んでSRVを作成
				D3D11_TEXTURE2D_DESC texture2d_desc{};
				load_texture_from_file(
					device,
					tex_path.c_str(),
					material_data.shader_resource_views[tex_idx].GetAddressOf(),
					&texture2d_desc
				);
			}
			else
			{
				//テクスチャがない場合はダミーテクスチャを作成
				uint32_t dummy_color = (tex_idx == 1) ? 0xFFFF7F7F : 0xFFFFFFFF;
				make_dummy_texture(
					device,
					material_data.shader_resource_views[tex_idx].GetAddressOf(),
					dummy_color,
					16
				);
			}
		}


		//-------------------------
		//解析したマテリアルを登録
		//-------------------------
		materials[material_data.unique_id] = std::move(material_data);
	}
}

//スケルトンの抽出
void SkinnedMeshResource::FetchSkeleton(fbxsdk::FbxScene* scene)
{
	int node_count = scene->GetSrcObjectCount<FbxNode>();
	std::unordered_map<std::string, int64_t> bone_name_to_index;
	for (int i = 0; i < node_count; i++)
	{
		FbxNode* node = scene->GetSrcObject<FbxNode>(i);
		FbxNodeAttribute* attribute = node->GetNodeAttribute();

		//-------------------------------------------
		//ノード属性がスケルトン(ボーン)であるか確認
		//-------------------------------------------
		if (attribute && attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton ||
			attribute->GetAttributeType() == FbxNodeAttribute::eNull ||
			attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			BoneData bone;
			bone.name = node->GetName();
			bone.parent_index = -1;

			//オフセット行列を計算して設定
			FbxAMatrix global = node->EvaluateGlobalTransform();
			bone.offset_transform = ToXMFloat4x4(global.Inverse());

			bones.push_back(std::move(bone));

			bone_name_to_index[node->GetName()] = bones.size() - 1;
		}
	}

	for (auto& bone : bones)
	{
		std::unordered_map<std::string, FbxNode*> node_cache;
		for (int i = 0; i < node_count; i++)
		{
			FbxNode* node = scene->GetSrcObject<FbxNode>(i);
			node_cache[node->GetName()] = node;
		}

		for (size_t i = 0; i < bones.size(); i++)
		{
			//ボーン名からFBXノードを取得
			FbxNode* node = node_cache[bone.name];

			if (node) continue;

			FbxNode* parent_node = node->GetParent();

			//親が存在し、親もボーンリストに含まれているか確認
			if (parent_node && bone_name_to_index.count(parent_node->GetName()) > 0)
			{
				bone.parent_index = bone_name_to_index[parent_node->GetName()];
			}
			else
			{
				bone.parent_index = -1;//親がいない
			}
		}
	}
}

//メッシュデータの抽出
void SkinnedMeshResource::FetchMeshes(
	ID3D11Device* device,
	fbxsdk::FbxScene* scene)
{
	//-------------------------------------
	//シーンからMesh属性を持つノードを収集
	//-------------------------------------
	int geometry_count = scene->GetGeometryCount();
	for (int i = 0; i < geometry_count; i++)
	{
		FbxGeometry* geometry = scene->GetGeometry(i);
		if (geometry->GetAttributeType() != FbxNodeAttribute::eMesh)continue;

		FbxMesh* fbx_mesh = static_cast<FbxMesh*>(geometry);
		FbxNode* node = fbx_mesh->GetNode();

		MeshData mesh_data;
		mesh_data.name = node->GetName();
		mesh_data.unique_id = node->GetUniqueID();
		mesh_data.default_global_transform = ToXMFloat4x4(node->EvaluateGlobalTransform());

		//---------
		//頂点情報
		//---------
		std::vector<BoneInfluencePerControlPoint> influences;
		FetchBoneInfluences(fbx_mesh, influences);

		bool has_skin = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin) > 0;

		int self_bone_index = -1;
		if (!has_skin)
		{
			std::string node_name = node->GetName();
			for (size_t b = 0; b < bones.size(); b++)
			{
				if (bones[b].name == node_name)
				{
					self_bone_index = static_cast<int>(b);
					break;
				}
			}
		}
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;

		//-----------------
		//ポリゴン数ループ
		//-----------------
		int polygon_count = fbx_mesh->GetPolygonCount();
		FbxVector4* control_points = fbx_mesh->GetControlPoints();

		//---------------------------------
		//マテリアルごとのサブセットを準備
		//---------------------------------
		int material_count = node->GetMaterialCount();
		if (material_count > 0)
		{
			mesh_data.subsets.resize(material_count);

			mesh_data.subsets.reserve(material_count > 0 ? material_count : 1);

			//-------------------
			//サブセットの初期化
			//-------------------
			for (int m = 0; m < material_count; m++)
			{
				FbxSurfaceMaterial* mat = node->GetMaterial(m);
				if (mat)
				{
					mesh_data.subsets[m].material_unique_id = mat->GetUniqueID();
					mesh_data.subsets[m].material_name = mat->GetName();
				}
				else
				{
					mesh_data.subsets[m].material_name = "Unknown";
				}
			}
		}
		else
		{
			mesh_data.subsets.resize(1);
			mesh_data.subsets[0].material_name = "Dammy";
		}

		//-----------------
		//メモリの事前確保
		//-----------------
		mesh_data.vertices.reserve(polygon_count * 3);
		mesh_data.indices.reserve(polygon_count * 3);
		std::unordered_map<int, std::vector<uint32_t>> unique_vertices;//重複チェック用のマップ

		//-------------------
		//ポリゴンごとの処理
		//-------------------
		for (int p = 0; p < polygon_count; p++)
		{
			//---------------------------------------
			//ポリゴンのマテリアルインデックスを取得
			//---------------------------------------
			int material_index = 0;
			if (material_count > 0)
			{
				FbxLayerElementMaterial* element_mat = fbx_mesh->GetElementMaterial();
				if (element_mat)
				{
					material_index = element_mat->GetIndexArray().GetAt(p);
				}
			}

			//--------------------
			//三角形の3頂点を処理
			//--------------------
			for (int v = 0; v < 3; v++)
			{
				//-----------------------------------
				//コントロールポイントのインデックス
				//-----------------------------------
				int ctrl_point_index = fbx_mesh->GetPolygonVertex(p, v);

				MeshVertex vertex;

				//---------
				//座標取得
				//---------
				vertex.position = ToXMFloat3(control_points[ctrl_point_index]);

				//---------
				//法線取得
				//---------
				FbxVector4 normal;
				fbx_mesh->GetPolygonVertexNormal(p, v, normal);
				vertex.normal = ToXMFloat3(normal);

				if (!has_skin && self_bone_index != -1)
				{
					//座標変換
					DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&vertex.position);
					DirectX::XMMATRIX M = DirectX::XMLoadFloat4x4(&mesh_data.default_global_transform);
					DirectX::XMStoreFloat3(&vertex.position, DirectX::XMVector3TransformCoord(P, M));

					//法線変換
					DirectX::XMVECTOR N = DirectX::XMLoadFloat3(&vertex.normal);
					DirectX::XMStoreFloat3(&vertex.normal, DirectX::XMVector3TransformNormal(N, M));
				}


				//-----------------------------
				//接線データが無ければ生成する
				//-----------------------------
				if (fbx_mesh->GetElementTangentCount() == 0)
				{
					fbx_mesh->GenerateTangentsData(0, false);
				}

				//-----------------
				//接線データの取得
				//-----------------
				if (fbx_mesh->GetElementTangentCount() > 0)
				{
					const FbxGeometryElementTangent* tangent = fbx_mesh->GetElementTangent(0);
					int tangent_index = 0;
					if (tangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
					{
						tangent_index = fbx_mesh->GetTextureUVIndex(p, v);
					}
					else if (tangent->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						tangent_index = ctrl_point_index;
					}
					int vertix_id_in_polygon = p * 3 + v;

					vertex.tangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(vertix_id_in_polygon)[0]);
					vertex.tangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(vertix_id_in_polygon)[1]);
					vertex.tangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(vertix_id_in_polygon)[2]);
					vertex.tangent.w = static_cast<float>(tangent->GetDirectArray().GetAt(vertix_id_in_polygon)[3]);
				}


				//-------
				//UV取得
				//-------
				FbxStringList uv_names;
				fbx_mesh->GetUVSetNames(uv_names);
				if (uv_names.GetCount() > 0)
				{
					FbxVector2 uv;
					bool unmapped;
					fbx_mesh->GetPolygonVertexUV(p, v, uv_names[0], uv, unmapped);
					vertex.texcoord.x = static_cast<float>(uv[0]);
					vertex.texcoord.y = 1.0f - static_cast<float>(uv[1]);
				}

				//---------------------
				//ボーンウェイトの適用
				//---------------------
				if (has_skin && ctrl_point_index < influences.size())
				{
					// 通常のスキニング処理
					const auto& influence_list = influences[ctrl_point_index];
					for (size_t k = 0; k < influence_list.size() && k < 4; k++)
					{
						vertex.bone_indices[k] = influence_list[k].bone_index;
						vertex.bone_weights[k] = influence_list[k].weight;
					}
				}
				else if (self_bone_index != -1)
				{
					// スキニング情報がない場合、特定した親ボーンに100%追従させる
					vertex.bone_indices[0] = self_bone_index;
					vertex.bone_weights[0] = 1.0f;
				}

				//-------------------
				//頂点の重複チェック
				//-------------------
				uint32_t index = 0;
				bool found = false;

				//同じ位置を持つ頂点が存在するかチェック
				if (unique_vertices.count(ctrl_point_index) > 0)
				{
					//候補の頂点リストを取得
					std::vector<uint32_t> candidates = unique_vertices[ctrl_point_index];

					for (uint32_t candidate_index : candidates)
					{
						const MeshVertex& exissting = mesh_data.vertices[candidate_index];

						//法線とUVがほぼ同じであれば、同じ頂点とみなして再利用
						const float epsilon = 1.0e-5f;//許容誤差

						bool normal_match =
							fabs(exissting.normal.x - vertex.normal.x) < epsilon &&
							fabs(exissting.normal.y - vertex.normal.y) < epsilon &&
							fabs(exissting.normal.z - vertex.normal.z) < epsilon;

						bool uv_match =
							fabs(exissting.texcoord.x - vertex.texcoord.x) < epsilon &&
							fabs(exissting.texcoord.y - vertex.texcoord.y) < epsilon;

						if (normal_match && uv_match)
						{
							index = candidate_index;
							found = true;
							break;
						}
					}
				}

				if (found)
				{
					//既存の頂点を再利用
					mesh_data.indices.push_back(index);
				}
				else
				{
					//新しい頂点として登録
					mesh_data.vertices.push_back(vertex);
					uint32_t new_index = static_cast<uint32_t>(mesh_data.vertices.size() - 1);
					mesh_data.indices.push_back(new_index);

					//検索用マップにも登録
					unique_vertices[ctrl_point_index].push_back(new_index);

					//バウンディングボックスの更新
					//最小値の更新
					mesh_data.bounding_box[0].x = (std::min)(mesh_data.bounding_box[0].x, vertex.position.x);
					mesh_data.bounding_box[0].y = (std::min)(mesh_data.bounding_box[0].y, vertex.position.y);
					mesh_data.bounding_box[0].z = (std::min)(mesh_data.bounding_box[0].z, vertex.position.z);

					//最大値の更新
					mesh_data.bounding_box[1].x = (std::max)(mesh_data.bounding_box[1].x, vertex.position.x);
					mesh_data.bounding_box[1].y = (std::max)(mesh_data.bounding_box[1].y, vertex.position.y);
					mesh_data.bounding_box[1].z = (std::max)(mesh_data.bounding_box[1].z, vertex.position.z);

				}

				//-----------------------------------------
				//サブセットのインデックスカウントを増やす
				//-----------------------------------------
				//マテリアルインデックスが範囲内かチェック
				if (material_index < mesh_data.subsets.size())
				{
					mesh_data.subsets[material_index].index_count++;
				}
			}
		}
		//---------------------------
		//サブセットの開始位置を計算
		//---------------------------
		uint32_t offset = 0;
		for (auto& subset : mesh_data.subsets)
		{
			subset.start_index_location = offset;
			offset += subset.index_count;
		}

		meshes.push_back(std::move(mesh_data));

		CreateComBuffers(device, meshes.back());
	}
}

//アニメーションデータの抽出
void SkinnedMeshResource::FetchAnimations(fbxsdk::FbxScene* scene, float sampling_rate)
{
	//-------------------------------------------
	//アニメーションスタック(クリップ)の数を取得
	//-------------------------------------------
	int stack_count = scene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < stack_count; i++)
	{
		FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(i);
		scene->SetCurrentAnimationStack(stack);
		AnimationClip clip;
		clip.name = stack->GetName();

		//-------------------------------
		//アニメーションの時間範囲を取得
		//-------------------------------
		FbxTakeInfo* take_info = scene->GetTakeInfo(clip.name.c_str());
		FbxTime start = take_info->mLocalTimeSpan.GetStart();
		FbxTime end = take_info->mLocalTimeSpan.GetStop();

		//-------------------------
		//サンプリングレートの設定
		//-------------------------
		FbxTime duration = end - start;
		FbxTime step;
		float valid_sampling_rate = sampling_rate > 0 ? sampling_rate : 24.0f;
		FbxTime one_second;
		one_second.SetTime(0, 0, 1, 0, 0, scene->GetGlobalSettings().GetTimeMode());
		step = one_second / static_cast<double>(valid_sampling_rate);

		//------------------------------------------------
		//ボーン名とFBXノードの対応を事前にキャッシュする
		//------------------------------------------------
		std::unordered_map<std::string, FbxNode*> node_cache;
		int node_count = scene->GetSrcObjectCount<FbxNode>();
		for (int i = 0; i < node_count; i++)
		{
			FbxNode* node = scene->GetSrcObject<FbxNode>(i);
			node_cache[node->GetName()] = node;
		}

		//-------------------
		//フレームごとの処理(簡易実装)
		//-------------------
		for (FbxTime t = start; t < end; t += step)
		{
			std::vector<AnimationKeyframeNode> keyframe_nodes;
			keyframe_nodes.resize(bones.size());

			for (size_t b = 0; b < bones.size(); b++)
			{
				//---------------------------
				//キャッシュからノードを取得
				//---------------------------

				//アニメーションが存在するかを確認
				auto it = node_cache.find(bones[b].name);
				if (it != node_cache.end())
				{
					FbxNode* node = it->second;

					//---------------------
					//グローバル行列の取得
					//---------------------
					FbxAMatrix global_transform = node->EvaluateGlobalTransform(t);
					keyframe_nodes[b].global_transform = ToXMFloat4x4(global_transform);

					keyframe_nodes[b].scaling = ToXMFloat3(global_transform.GetS());
					keyframe_nodes[b].rotation = ToXMFloat4(global_transform.GetQ());
					keyframe_nodes[b].translation = ToXMFloat3(global_transform.GetT());
				}
			}
			clip.sequence.push_back(std::move(keyframe_nodes));
		}
		animations[clip.name] = std::move(clip);
	}
}

//特定のメッシュの頂点に対するボーンの影響度を抽出
void SkinnedMeshResource::FetchBoneInfluences(
	const fbxsdk::FbxMesh* fbx_mesh,
	std::vector<BoneInfluencePerControlPoint>& influences)
{
	//-------------------------------------------------------
	//コントロールポイント(頂点座標の数)に合わせてサイズ確保
	//-------------------------------------------------------
	int control_points_count = fbx_mesh->GetControlPointsCount();
	influences.resize(control_points_count);

	//-----------------------------------------------------
	//スキンデフォーマ(メッシュを変形させる情報)の数を取得
	//-----------------------------------------------------
	int skin_count = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);

	for (int i = 0; i < skin_count; i++)
	{
		//-------------
		//スキンを取得
		//-------------
		FbxSkin* skin = static_cast<FbxSkin*>(fbx_mesh->GetDeformer(i, FbxDeformer::eSkin));

		//-----------------------------------------
		//クラスタ(ボーンごとの影響情報)の数を取得
		//-----------------------------------------
		int cluster_count = skin->GetClusterCount();
		
		for (int j = 0; j < cluster_count; j++)
		{
			FbxCluster* cluster = skin->GetCluster(j);

			//-------------------------------------------
			//クラスタに関連付けられたボーンノードを取得
			//-------------------------------------------
			FbxNode* link_node = cluster->GetLink();
			if (!link_node) continue;

			//-------------------------------------------------------
			//ボーンリストから、このノードに対応すインデックスを検索
			//-------------------------------------------------------
			int bone_index = -1;
			std::string bone_name = link_node->GetName();

			for (size_t k = 0; k < bones.size(); k++)
			{
				if (bones[k].name == bone_name)
				{
					bone_index = static_cast<int>(k);

					//-----------------------------------------------------
					//バインドポーズの逆行列(オフセット行列)を取得して保存
					//-----------------------------------------------------
					FbxAMatrix transform_matrix;
					cluster->GetTransformMatrix(transform_matrix);

					FbxAMatrix transform_link_matrix;
					cluster->GetTransformLinkMatrix(transform_link_matrix);

					//-------------------------------------------------------
					//逆バインドポーズ行列 = ポーズの逆行列 * メッシュの行列
					//-------------------------------------------------------
					FbxAMatrix inverse_bind_pose = transform_link_matrix.Inverse() * transform_matrix;
					bones[k].offset_transform = ToXMFloat4x4(inverse_bind_pose);

					break;
				}
			}
			//---------------------------------
			//ボーンが見つからなけらばスキップ
			//---------------------------------
			if (bone_index == -1) continue;

			//-----------------------------------------------------
			//ボーンの影響を受ける頂点インデックスとウェイトを取得
			//-----------------------------------------------------
			int index_count = cluster->GetControlPointIndicesCount();
			int* indices = cluster->GetControlPointIndices();
			double* weights = cluster->GetControlPointWeights();

			for (int k = 0; k < index_count; k++)
			{
				int control_point_index = indices[k];
				float weight = static_cast<float>(weights[k]);

				//---------------
				//影響情報を追加
				//---------------
				BoneInfluence influence;
				influence.bone_index = static_cast<uint32_t>(bone_index);
				influence.weight = weight;

				influences[control_point_index].push_back(influence);
			}
		}
	}
}

//読み込んだ頂点データを、DirectX（GPU）が使えるバッファに変換
void SkinnedMeshResource::CreateComBuffers(
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
	device->CreateBuffer(&buffer_desc, &subresource_data, mesh.vertex_buffer.ReleaseAndGetAddressOf());

	//---------------------------
	//インデックスバッファの作成
	//---------------------------
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	subresource_data.pSysMem = mesh.indices.data();
	device->CreateBuffer(&buffer_desc, &subresource_data, mesh.index_buffer.ReleaseAndGetAddressOf());
}

//Resource(モデル)を受け取って初期化
SkinnedMeshComponent::SkinnedMeshComponent(std::shared_ptr<SkinnedMeshResource> resource)
	:resource(resource)
{
	//-------------------------------------
	//ボーン行列用配列のサイズ確保と初期化
	//-------------------------------------
	if (this->resource)
	{
		current_bone_transforms.resize(resource->GetBones().size());
		for (auto& m : current_bone_transforms)
		{
			DirectX::XMStoreFloat4x4(&m, DirectX::XMMatrixIdentity());
		}
	}
}

//アニメーションを更新
void SkinnedMeshComponent::Update(float delta_time)
{
	if (!resource || current_clip_name.empty()) return;

	//-------------
	//時間を進める
	//-------------
	const auto& clip = resource->GetAnimations().at(current_clip_name);
	current_time += delta_time * clip.sampling_rate;

	//-----------
	//ループ処理
	//-----------
	float max_time = static_cast<float>(clip.sequence.size());
	if (current_time >= max_time)
	{
		if (is_loop)
		{
			current_time = fmod(current_time, max_time);
		}
		else
		{
			current_time = max_time - 0.001f;
		}
	}

	//-----------------
	//ボーン行列の計算
	//-----------------
	CalculateBoneTransforms();
}

//GPUに現在の姿勢データを渡して描画指示を出す
void SkinnedMeshComponent::Render(
	ID3D11DeviceContext* context,
	const DirectX::XMFLOAT4X4 world_transform,
	const DirectX::XMFLOAT4 color)
{
	if (!resource)return;

	//-------------------
	//定数バッファの更新
	//-------------------

	//定数バッファ用のデータ構造
	struct Constants
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4 material_color;
		DirectX::XMFLOAT4X4 bone_transformes[256];
	}data{};

	data.world = world_transform;
	data.material_color = color;

	//計算済みのボーン行列をコピー
	for (size_t i = 0; i < current_bone_transforms.size() && i < 256; i++)
	{
		data.bone_transformes[i] = current_bone_transforms[i];
	}

	//定数バッファの更新
	context->UpdateSubresource(resource->GetConstantBuffer(), 0, nullptr, &data, 0, 0);

	//-----------------
	//パイプライン設定
	//-----------------
	context->VSSetShader(resource->GetVertexShader(), nullptr, 0);
	context->PSSetShader(resource->GetPixelShader(), nullptr, 0);

	ID3D11Buffer* cb = resource->GetConstantBuffer();
	context->VSSetConstantBuffers(0, 1, &cb);
	context->IASetInputLayout(resource->GetInputLayout());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//-------------------
	//メッシュごとの描画
	//-------------------
	for (const auto& mesh : resource->GetMeshes())
	{
		//頂点・インデックスバッファセット
		UINT stride = sizeof(MeshVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.vertex_buffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//サブセットごとの描画
		for (const auto& subset : mesh.subsets)
		{
			const auto* material = resource->GetMaterial(subset.material_unique_id);

			if (material)
			{
				//テクスチャ設定
				context->PSSetShaderResources(0, 1, material->shader_resource_views[0].GetAddressOf());
				context->PSSetShaderResources(1, 1, material->shader_resource_views[1].GetAddressOf());
			}

			//描画命令
			context->DrawIndexed(subset.index_count, subset.start_index_location, 0);
		}
	}
}

//再生するアニメーションを名前で切り替え
void SkinnedMeshComponent::PlayAnimation(const std::string& clip_name, bool loop)
{
	//-----------------------------------------------
	//リソースにその名前のアニメーションがあるか確認
	//-----------------------------------------------
	auto& anims = resource->GetAnimations();
	if (anims.find(clip_name) != anims.end())
	{
		current_clip_name = clip_name;
		current_time = 0.0f;
		is_loop = loop;
	}
}

//アニメーション計算用ヘルパー
void SkinnedMeshComponent::CalculateBoneTransforms()
{
	const auto& clip = resource->GetAnimations().at(current_clip_name);
	const auto& bones = resource->GetBones();

	//---------------------------------------------
	//現在の時間に対応する2つのキーフレームを取得
	//---------------------------------------------
	size_t frame_index_curr = static_cast<size_t>(current_time);	//現在のフレーム番号
	size_t frame_index_next = frame_index_curr + 1;					//次のフレーム番号
	float t = current_time - static_cast<float>(frame_index_curr);	//補完係数(0.0～1.0)
	t = (std::max)(0.0f, (std::min)(t, 1.0f));//0.0f～1.0fの範囲に収める

	//---------------------------------
	//ループ処理や範囲外アクセスの防止
	//---------------------------------
	if (frame_index_next >= clip.sequence.size())
	{
		if (is_loop)
		{
			frame_index_curr %= clip.sequence.size();
			frame_index_next %= clip.sequence.size();
		}
		else
		{
			frame_index_curr = clip.sequence.size() - 1;
			frame_index_next = clip.sequence.size() - 1;
		}
	}

	const auto& nodes_curr = clip.sequence[frame_index_curr];
	const auto& nodes_next = clip.sequence[frame_index_next];

	//-------------------------
	//全ボーンについて補完計算
	//-------------------------
	for (size_t i = 0; i < bones.size(); i++)
	{
		//スケールの線形補完(Lerp)
		DirectX::XMVECTOR S_curr = DirectX::XMLoadFloat3(&nodes_curr[i].scaling);
		DirectX::XMVECTOR S_next = DirectX::XMLoadFloat3(&nodes_next[i].scaling);
		DirectX::XMVECTOR S = DirectX::XMVectorLerp(S_curr, S_next, t);

		//回転の球面線形補完(Slerp)
		DirectX::XMVECTOR R_curr = DirectX::XMLoadFloat4(&nodes_curr[i].rotation);
		DirectX::XMVECTOR R_next = DirectX::XMLoadFloat4(&nodes_next[i].rotation);
		DirectX::XMVECTOR R = DirectX::XMQuaternionSlerp(R_curr, R_next, t);

		//位置の線形補完(Lerp)
		DirectX::XMVECTOR T_curr = DirectX::XMLoadFloat3(&nodes_curr[i].translation);
		DirectX::XMVECTOR T_next = DirectX::XMLoadFloat3(&nodes_next[i].translation);
		DirectX::XMVECTOR T = DirectX::XMVectorLerp(T_curr, T_next, t);

		//行列の再合成(S*R*T)
		DirectX::XMMATRIX global_transform =
			DirectX::XMMatrixScalingFromVector(S) * 
			DirectX::XMMatrixRotationQuaternion(R) * 
			DirectX::XMMatrixTranslationFromVector(T);

		//最終行列の計算(オフセット*グローバル)
		DirectX::XMMATRIX offset_transform = DirectX::XMLoadFloat4x4(&bones[i].offset_transform);
		DirectX::XMMATRIX final_transform = offset_transform * global_transform;

		DirectX::XMStoreFloat4x4(&current_bone_transforms[i], final_transform);
	}
}