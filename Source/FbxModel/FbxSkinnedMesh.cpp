#include "FbxAnimation.h"

#include <fbxsdk.h>
#include <algorithm>
#include <unordered_map>
#include <map>
#include "FbxSkinnedMesh.h"

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
}

//========================
//メッシュデータの抽出
//========================
void FbxSkinnedMesh::Fetch(
	ID3D11Device* device,
	fbxsdk::FbxScene* scene,
	const std::vector<BoneData>& bones,
	std::vector<MeshData>& out_meshes)
{
	//出力用マップをクリア
	out_meshes.clear();

	//シーン内のジオメトリ(形状データ)数を取得
	int geometry_count = scene->GetGeometryCount();

	//全ジオメトリについてループ
	for (int i = 0; i < geometry_count; i++)
	{
		//ジオメトリを取得
		FbxGeometry* geometry = scene->GetGeometry(i);

		//メッシュ属性でなければスキップ
		if (geometry->GetAttributeType() != FbxNodeAttribute::eMesh)continue;

		//FbxMesh型にキャスト
		FbxMesh* fbx_mesh = static_cast<FbxMesh*>(geometry);

		//紐づいているノードを取得
		FbxNode* node = fbx_mesh->GetNode();

		//メッシュデータ構造体を作成
		MeshData mesh_data;

		//ノードの名前とユニークIDをメッシュデータに保存
		mesh_data.name = node->GetName();
		mesh_data.unique_id = node->GetUniqueID();

		//初期のグローバル変換行列を取得
		mesh_data.default_global_transform = ToXMFloat4x4(node->EvaluateGlobalTransform());

		//--------------------
		//ウェイト情報の取得
		//--------------------
		std::vector<BoneInfluencePerControlPoint> influences;
		FetchBoneInfuences(fbx_mesh, bones, influences);

		//ウェイト情報が空でなければスキンメッシュとみなす
		bool has_skin = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin) > 0;

		//スキンがない場合、親ボーン(自分自身)を探す
		int self_bone_index = -1;
		if (!has_skin)
		{
			std::string node_name = node->GetName();

			//ボーンリストから自分の名前を探す
			for (size_t b = 0; b < bones.size(); b++)
			{
				if (bones[b].name == node_name)
				{
					self_bone_index = static_cast<int>(b);
					break;
				}
			}
		}

		//---------------------------------
		//マテリアルごとのサブセットを準備
		//---------------------------------
		int material_count = node->GetMaterialCount();
		if (material_count > 0)
		{
			//マテリアル数分サブセットを確保
			mesh_data.subsets.resize(material_count);

			mesh_data.subsets.reserve(material_count > 0 ? material_count : 1);

			//サブセットの初期化
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
			//マテリアルがない場合はダミーを一つ作る
			mesh_data.subsets.resize(1);
			mesh_data.subsets[0].material_name = "Dammy";
		}

		//-------------------------
		//ポリゴンの解析と頂点生成
		//-------------------------

		//接線データが無ければ生成する
		if (fbx_mesh->GetElementTangentCount() <= 0)
		{
			fbx_mesh->GenerateTangentsData(0, false);
		}

		int polygon_count = fbx_mesh->GetPolygonCount();
		FbxVector4* control_points = fbx_mesh->GetControlPoints();

		//メモリ予約(ポリゴン数*3頂点)
		mesh_data.vertices.reserve(polygon_count * 3);
		mesh_data.indices.reserve(polygon_count * 3);

		//重複チェック用のマップ
		std::unordered_map<int, std::vector<uint32_t>> unique_vertices;

		//全ポリゴンについてループ
		for (int p = 0; p < polygon_count; p++)
		{
			//現在のポリゴンのマテリアルインデックスを取得
			int material_index = 0;
			if (material_count > 0)
			{
				FbxLayerElementMaterial* element_mat = fbx_mesh->GetElementMaterial();
				if (element_mat)
				{
					material_index = element_mat->GetIndexArray().GetAt(p);
				}
			}

			//三角形の3頂点について処理
			for (int v = 0; v < 3; v++)
			{
				//コントロールポイントのインデックスを取得
				int ctrl_point_index = fbx_mesh->GetPolygonVertex(p, v);

				MeshVertex vertex;

				//位置情報を取得して変換
				vertex.position = ToXMFloat3(control_points[ctrl_point_index]);

				//法線ベクトルを取得して変換
				FbxVector4 normal;
				fbx_mesh->GetPolygonVertexNormal(p, v, normal);
				vertex.normal = ToXMFloat3(normal);

				//UV座標の取得
				FbxStringList uv_names;
				fbx_mesh->GetUVSetNames(uv_names);
				if (uv_names.GetCount() > 0)
				{
					FbxVector2 uv;
					bool unmapped;

					//最初のUVセットから取得
					fbx_mesh->GetPolygonVertexUV(p, v, uv_names[0], uv, unmapped);

					//V座標を反転してDirectX刑に合わせる
					vertex.texcoord.x = static_cast<float>(uv[0]);
					vertex.texcoord.y = 1.0f - static_cast<float>(uv[1]);
				}

				//-------------------
				//接線データの取得
				//-------------------
				//メッシュに接線データが含まれているかを確認
				if (fbx_mesh->GetElementTangentCount() > 0)
				{
					//最初の接線データレイヤー（レイヤー0）へのポインタを取得
					const FbxGeometryElementTangent* tangent = fbx_mesh->GetElementTangent(0);

					//最終的にデータを取得するための配列インデックスを格納
					int tangent_index = 0;

					//ポリゴンごとの頂点通し番号
					int vertex_count = p * 3 + v;

					//マッピングモード（データがどのように割り当てられているか）の判定
					if (tangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
					{
						//リファレンスモード（データの格納方法）の判定
						if (tangent->GetReferenceMode() == FbxGeometryElement::eDirect)
						{
							//頂点の通し番号をそのままデータ配列のインデックスとして使用
							tangent_index = vertex_count;
						}
						//eIndexToDirect: インデックス配列を経由してデータにアクセスする
						else if (tangent->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						{
							//インデックス配列から、実際のデータ位置を取得
							tangent_index = tangent->GetIndexArray().GetAt(vertex_count);
						}
					}
					//ByControlPoint: 制御点（共有頂点）ごとに接線を持つ（滑らかな曲面などで使われる）
					else if (tangent->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						//eDirect: 制御点のインデックス順にデータが並んでいる
						if (tangent->GetReferenceMode() == FbxGeometryElement::eDirect)
						{
							//コントロールポイントのインデックスをそのまま使用
							tangent_index = ctrl_point_index;
						}
						//eIndexToDirect: インデックス配列を経由する
						else if (tangent->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						{
							//インデックス配列から取得
							tangent_index = tangent->GetIndexArray().GetAt(ctrl_point_index);
						}
					}

					//確定したインデックスを使って、接線データ配列からベクトル(x,y,z,w)を取得
					FbxVector4 t = tangent->GetDirectArray().GetAt(tangent_index);

					//取得した接線データを、DirectX用の頂点構造体にキャストして代入
					vertex.tangent.x = static_cast<float>(t[0]);
					vertex.tangent.y = static_cast<float>(t[1]);
					vertex.tangent.z = static_cast<float>(t[2]);
					vertex.tangent.w = static_cast<float>(t[3]);
				}

				//---------------------
				//ウェイト情報の適用
				//---------------------
				if (has_skin && ctrl_point_index < influences.size())
				{
					//抽出済みのウェイト情報を適用
					const auto& influence_list = influences[ctrl_point_index];

					//最大4ボーンまで
					for (size_t k = 0; k < influence_list.size() && k < 4; k++)
					{
						vertex.bone_indices[k] = influence_list[k].bone_index;
						vertex.bone_weights[k] = influence_list[k].weight;
					}
				}
				else if (self_bone_index != -1)
				{
					// スキニング情報がない場合、自分自身のボーンに100%追従させる
					vertex.bone_indices[0] = self_bone_index;
					vertex.bone_weights[0] = 1.0f;
				}

				//-------------------
				//頂点の登録
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

		//サブセットの開始位置を計算
		uint32_t offset = 0;
		for (auto& subset : mesh_data.subsets)
		{
			subset.start_index_location = offset;
			offset += subset.index_count;
		}

		//GPUバッファの作成
		CreateComBuffers(device, mesh_data);
		
		//完成したメッシュデータをリストに追加
		out_meshes.push_back(std::move(mesh_data));
	}
}

//===================
//ウェイト情報の保持
//===================
void FbxSkinnedMesh::FetchBoneInfuences(
	const fbxsdk::FbxMesh* fbx_mesh,
	const std::vector<BoneData>& bones,
	std::vector<BoneInfluencePerControlPoint>& influences)
{
	//コントロールポイント数(頂点座標の数)に合わせてサイズ確保
	int control_points_count = fbx_mesh->GetControlPointsCount();
	influences.resize(control_points_count);

	//スキンデフォーマ(メッシュを変形させる情報)の数を取得
	int skin_count = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);

	for (int i = 0; i < skin_count; i++)
	{
		//スキンを取得
		FbxSkin* skin = static_cast<FbxSkin*>(fbx_mesh->GetDeformer(i, FbxDeformer::eSkin));

		//クラスタ(ボーンごとの影響情報)の数を取得
		int cluster_count = skin->GetClusterCount();

		for (int j = 0; j < cluster_count; j++)
		{
			FbxCluster* cluster = skin->GetCluster(j);

			//クラスタに関連付けられたボーンノードを取得
			FbxNode* link_node = cluster->GetLink();
			if (!link_node) continue;

			//ボーンリストから、現在のノードに対応すインデックスを検索
			int bone_index = -1;
			std::string bone_name = link_node->GetName();

			for (size_t k = 0; k < bones.size(); k++)
			{
				if (bones[k].name == bone_name)
				{
					bone_index = static_cast<int>(k);
					break;
				}
			}

			//ボーンが見つからなけらばスキップ
			if (bone_index == -1) continue;

			//ボーンの影響を受ける頂点インデックスとウェイトを取得
			int index_count = cluster->GetControlPointIndicesCount();
			int* indices = cluster->GetControlPointIndices();
			double* weights = cluster->GetControlPointWeights();

			//各頂点にウェイト情報を追加
			for (int k = 0; k < index_count; k++)
			{
				int control_point_index = indices[k];
				float weight = static_cast<float>(weights[k]);

				BoneInfluence influence;
				influence.bone_index = static_cast<uint32_t>(bone_index);
				influence.weight = weight;

				//該当するコントロールポインタのリストに追加
				influences[control_point_index].push_back(influence);
			}
		}
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
	//サイズ計算
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(MeshVertex) * mesh.vertices.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	//データポインタ設定
	subresource_data.pSysMem = mesh.vertices.data();

	//バッファの作成
	device->CreateBuffer(&buffer_desc, &subresource_data, mesh.vertex_buffer.ReleaseAndGetAddressOf());

	//---------------------------
	//インデックスバッファの作成
	//---------------------------
	//サイズ計算
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	//データポインタ設定
	subresource_data.pSysMem = mesh.indices.data();

	//バッファの作成
	device->CreateBuffer(&buffer_desc, &subresource_data, mesh.index_buffer.ReleaseAndGetAddressOf());

}
