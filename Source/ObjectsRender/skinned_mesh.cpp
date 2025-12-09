#include "skinned_mesh.h"
#include "misc.h"
#include <sstream>
#include <fstream>
#include <functional>
#include "shader.h"

using namespace DirectX;

/*異なる数学ライブラリ間で行列データを正しく受け渡すための橋渡し関数*/
inline XMFLOAT4X4 skinned_mesh::to_xmfloat4x4(const FbxAMatrix& fbxamatrix)
{
	//DirectX用の4x4行列変数を宣言します。ここに変換結果を格納
	XMFLOAT4X4 xmfloat4x4;
	//行列は4行4列なので、行（row）を0から3までループ
	for (int row = 0; row < 4; row++)
	{
		//各行について、列（column）を0から3までループ
		for (int column = 0; column < 4; column++)
		{
			// fbxamatrix[row][column] でFBX行列の特定の場所の数値を取得し、
			// それを static_cast<float> で単精度浮動小数点数（float）に変換
			// そして、xmfloat4x4.m[row][column] に格納します。
			// 'm' は DirectXMath の行列が持つ内部配列を指します。
			xmfloat4x4.m[row][column] = static_cast<float>(fbxamatrix[row][column]);
		}
	}
	//変換されたDirectX形式の行列を返す
	return xmfloat4x4;
}

/*FBX SDKが扱う3次元ベクトル（または点）のデータ形式を、
DirectXが使う3次元ベクトルのデータ形式に変換*/
inline XMFLOAT3 skinned_mesh::to_xmfloat3(const FbxDouble3& fbxdouble3)
{
	//DirectX用の3次元ベクトル変数を宣言します。ここに変換結果を格納
	XMFLOAT3 xmfloat3;

	/*FBXの3次元ベクトルの最初の要素（X,Y,Z成分）を取得し、
	static_cast<float> で単精度浮動小数点数（float）に変換*/
	xmfloat3.x = static_cast<float>(fbxdouble3[0]);
	xmfloat3.y = static_cast<float>(fbxdouble3[1]);
	xmfloat3.z = static_cast<float>(fbxdouble3[2]);

	//変換されたDirectX形式の3次元ベクトルを返す
	return xmfloat3;
}

/*異なるライブラリ間で色情報又は3次元座標とW成分を変換する関数*/
inline XMFLOAT4 skinned_mesh::to_xmfloat4(const FbxDouble4& fbxdouble4)
{
	/*DirectX用の4次元ベクトル（または色情報）変数を宣言します。
	ここに変換結果を格納*/
	XMFLOAT4 xmfloat4;

	/*FBXの4次元ベクトルの最初の要素を取得し（R,G,B,A又はX,Y,Z,W）、
	  static_cast<float> で単精度浮動小数点数（float）に変換*/
	xmfloat4.x = static_cast<float>(fbxdouble4[0]);
	xmfloat4.y = static_cast<float>(fbxdouble4[1]);
	xmfloat4.z = static_cast<float>(fbxdouble4[2]);
	xmfloat4.w = static_cast<float>(fbxdouble4[3]);

	/*変換されたDirectX形式の4次元ベクトル（または色情報）を返す*/
	return xmfloat4;
}



//特定のボーンが、モデルの1つの頂点にどれくらい影響を与えるかを定義
struct bone_influence
{
	//影響を与えるボーンの識別番号（インデックス）
	uint32_t bone_index;

	//ボーンが頂点に与える影響の強さ（重み）
	float bone_weight;
};


/*「1つのコントロールポイント（頂点）が、
複数のボーンからどれくらい影響を受けるか」 を表現するための型定義*/
using bone_influence_per_control_point = std::vector<bone_influence>;

/*3Dモデルの各頂点（点）が、
「どの骨（ボーン）によって、どれくらいの割合で動かされるか」という情報を、
FBXファイルから読み取って、プログラムで使える形にまとめる処理*/
void fetch_bone_influences(
	const FbxMesh* fbx_mesh,
	std::vector<bone_influence_per_control_point>& bone_influences)
{
	/*FBXメッシュが持っている頂点（コントロールポイント）の総数を取得*/
	const int control_points_count{ fbx_mesh->GetControlPointsCount() };

	/*抽出したボーンの影響情報を格納するための
	bone_influences 配列のサイズを、頂点の総数に合わせて確保*/
	bone_influences.resize(control_points_count);

	/*このメッシュに適用されている「スキンデフォーマ」の数を取得*/
	const int skin_count{ fbx_mesh->GetDeformerCount(FbxDeformer::eSkin) };

	//見つかったすべてのスキンデフォーマについてループ
	for (int skin_index = 0; skin_index < skin_count; skin_index++)
	{
		/*指定されたインデックスとタイプ（ここではeSkin）のデフォーマを取得*/
		const FbxSkin* fbx_skin
		{ static_cast<FbxSkin*>(fbx_mesh->GetDeformer(
			skin_index,FbxDeformer::eSkin)) };

		//クラスターの総数を取得
		const int cluster_count{ fbx_skin->GetClusterCount() };

		//見つかったすべてのクラスターについてループ
		for (int cluster_index = 0; cluster_index < cluster_count; cluster_index++)
		{
			//現在のインデックスのクラスターを取得
			const FbxCluster* fbx_cluster{ fbx_skin->GetCluster(cluster_index) };

			/*クラスターが影響を与える頂点（コントロールポイント）の数を取得*/
			const int control_point_indices_count
			{ fbx_cluster->GetControlPointIndicesCount() };

			for (int control_point_indeices_index = 0;
				control_point_indeices_index < control_point_indices_count;
				control_point_indeices_index++)
			{
				/*クラスターが影響を与えるコントロールポイントの
				インデックスの配列を取得*/
				int control_point_index{ fbx_cluster->
					GetControlPointIndices()[control_point_indeices_index] };
				/*クラスターが影響を与える各コントロールポイントに対応する
				重み（影響度）の配列を取得*/
				double control_point_weight
				{ fbx_cluster->GetControlPointWeights()[control_point_indeices_index] };

				/*現在処理しているコントロールポイントに対応する
				bone_influence_per_control_pointにアクセスして
				構造体の末尾に追加*/
				bone_influence& bone_influence
				{ bone_influences.at(control_point_index).emplace_back() };

				/*現在処理しているクラスター（＝ボーン）のインデックスを格納*/
				bone_influence.bone_index = static_cast<uint32_t>(cluster_index);

				/*取得したコントロールポイントの重みを格納*/
				bone_influence.bone_weight = static_cast<float>(control_point_weight);
			}
		}
	}
}

skinned_mesh::skinned_mesh(
	ID3D11Device* device,
	const char* fbx_filename,
	bool triangulate,
	float sampling_rate)
{
	std::filesystem::path cereal_filename(fbx_filename); 
	cereal_filename.replace_extension("cerreal");
	if (std::filesystem::exists(cereal_filename.c_str()))
	{
		std::ifstream ifs(cereal_filename.c_str(), std::ios::binary);
		cereal::BinaryInputArchive deserialization(ifs);
		deserialization(
			scene_view,
			meshes,
			materials,
			animation_clips
			);
	}
	else
	{
		//ダミーマテリアルを初期化
		//create_dummy_material(device);

		//FBX SDKの初期化とファイルの読み込み


		// FBX 全体の管理を作成
		FbxManager* fbx_manager{ FbxManager::Create() };

		// FBX のシーン（モデルとかカメラとかいろいろ）なものを呼び出すための箱）を作成
		FbxScene* fbx_scene{ FbxScene::Create(fbx_manager,"") };

		// FBX 外部ファイルからの呼び出し用クラスを作成
		FbxImporter* fbx_importer{ FbxImporter::Create(fbx_manager,"") };

		// FBX 外部ファイルを読み込んで初期化
		bool import_status{ false };
		import_status = fbx_importer->Initialize(fbx_filename);
		_ASSERT_EXPR_A(import_status, fbx_importer->GetStatus().GetErrorString());


		//メモリを読み込みfbx_sceneオブジェクトに格納する
		import_status = fbx_importer->Import(fbx_scene);
		_ASSERT_EXPR_A(import_status, fbx_importer->GetStatus().GetErrorString());



		//メッシュの形状変換（ポリゴンを三角形に変換）


		//メッシュに対して様々な変換や修正を行う(ポリゴンを三角形に変換など)
		FbxGeometryConverter fbx_converter(fbx_manager);

		//FBXのモデルのポリゴンを三角形に変換するかどうか
		if (triangulate)
		{
			//fbx_scene内のすべてのポリゴンを三角形に変換
			fbx_converter.Triangulate(fbx_scene, true/*replace*/, false/*legacy*/);

			//不正なポリゴンを削除
			fbx_converter.RemoveBadPolygonsFromMeshes(fbx_scene);
		}



		//FBXシーンのノード構造を解析し、カスタムデータ構造に格納


		//FBXのノードを受け取り、その情報を処理する
		std::function<void(FbxNode*)> traverse{ [&](FbxNode* fbx_node)
			//FBXノードを格納するための箱をリストの最後に追加
			{         scene::node& node{ scene_view.nodes.emplace_back() };

		//現在のFBXノードの種類(3Dモデルのメッシュやカメラ、ライトなど)を調べる
		node.attribute = fbx_node->GetNodeAttribute() ?
			fbx_node->GetNodeAttribute()->
			//属性を持っていたらそれを取得/属性が無ければ不明な種類とする
			GetAttributeType() : FbxNodeAttribute::EType::eUnknown;

		//現在のノードの名前を保存
		node.name = fbx_node->GetName();

		//現在のノードののユニークID(他と重複しない唯一の識別番号)を保存
		node.unique_id = fbx_node->GetUniqueID();

		/*現在のノードの親ノードを調べて、
		それがどこに格納(何番目にあるのか)されてるかを保存*/
		node.parent_index = scene_view.indexof(fbx_node->GetParent() ?
			fbx_node->GetParent()->GetUniqueID() : 0);

		//現在のノードに子ノードがあるかを調べる
		for (int child_index = 0; child_index < fbx_node->GetChildCount(); child_index++)
		{
			traverse(fbx_node->GetChild(child_index));
		}
		} };

		//シーン全体のノードを順に処理して、scene_view.nodesリストに格納する
		traverse(fbx_scene->GetRootNode());

		//メッシュデータの抽出
		fetch_meshes(fbx_scene, meshes);

		//マテリアルデータを抽出
		fetch_materials(fbx_scene, materials);

		//サンプリングレート（アニメーションをどれだけの頻度でサンプリングするか）を0で初期化
		//float sampling_rate = 0;

		//アニメーションのキーフレームを取得
		fetch_animations(fbx_scene, animation_clips, sampling_rate);

#if 1


		//デバッグ情報の出力


		//scene_view.nodesのリストに保存されているノードをすべて処理する
		for (const scene::node& node : scene_view.nodes)
		{
			//fbx_sceneの中から対応するノードを取り出す
			FbxNode* fbx_node{ fbx_scene->FindNodeByName(node.name.c_str()) };

			//Display node data in the output window as debug

			//見つけたFBXノードの名前を取得して保存
			std::string node_name = fbx_node->GetName();

			//見つけたFBXノードのユニークIDを取得して保存
			uint64_t uid = fbx_node->GetUniqueID();

			//デバック情報としてノードの親子関係を把握する
			uint64_t parent_uis = fbx_node->GetParent() ?
				fbx_node->GetParent()->GetUniqueID() : 0;

			//見つけたFBXノードの属性のタイプを取得して保存
			int32_t type = fbx_node->GetNodeAttribute() ?
				fbx_node->GetNodeAttribute()->GetAttributeType() : 0;


			std::stringstream debug_string;
			debug_string << node_name << ":" << uid << ":" << parent_uis << ":" << type << "\n";

			//画面にFBXノードの情報を表示
			OutputDebugStringA(debug_string.str().c_str());
		}
#endif
		//FBXノードのすべてのリソースを開放
		fbx_manager->Destroy();

		std::ofstream ofs(cereal_filename.c_str(), std::ios::binary);
		cereal::BinaryOutputArchive serialization(ofs);
		serialization(scene_view, meshes, materials, animation_clips);
	}
	//DirectX COMオブジェクトの作成
	create_com_objects(device, fbx_filename);
}

//FBX SDKを使ってFBXシーンからメッシュデータを抽出し、
// std::vector<mesh>に格納する
void skinned_mesh::fetch_meshes(
	FbxScene* fbx_scene,
	std::vector<mesh>& meshes)
{
	//skinned_meshオブジェクトに読み込まれているscene_view.nodesを1つずつ調べる
	for (const scene::node & node : scene_view.nodes)
	{
		//メッシュ（ポリゴン）データを持つノードであるかを確認
		if (node.attribute != FbxNodeAttribute::EType::eMesh)
		{
			continue;
		}


		//FBXメッシュの取得


		//FbxNodeオブジェクトをfbx_sceneから検索して取得
		FbxNode* fbx_node{ fbx_scene->FindNodeByName(node.name.c_str()) };
		//FbxNodeが持つメッシュデータ（FbxMesh）を取得
		FbxMesh* fbx_mesh{ fbx_node->GetMesh() };


		//カスタムメッシュ構造体の初期化


		//meshesベクターの末尾に新しいmeshオブジェクトを追加し、その参照を取得
		mesh& mesh{ meshes.emplace_back() };

		//FBXノードから一意なIDと名前を取得して、作成中のmeshオブジェクトに設定
		mesh.unique_id = fbx_node->GetUniqueID();
		mesh.name = fbx_node->GetName();

		/*scene_viewのnodes配列のどの要素に関連付けられているかを示す
		インデックスを取得し、設定*/
		mesh.node_index = scene_view.indexof(mesh.unique_id);

		/*FBXファイルから読み込んだ3Dモデルのメッシュ（mesh）に対して、
そのメッシュの「デフォルトのグローバル変換行列」を設定*/
		mesh.default_global_transform =
			to_xmfloat4x4(fbx_node->EvaluateGlobalTransform());

		/*各頂点（コントロールポイント）がどのボーンにどれくらい影響されるか、
		という情報を保持するための入れ物*/
		std::vector<bone_influence_per_control_point>bone_influecnes;

		/*スキニングに必要なボーンの影響情報を bone_influecnes に抽出して格納*/
		fetch_bone_influences(fbx_mesh, bone_influecnes);

		fetch_skeleton(fbx_mesh, mesh.bind_pose);

		//mesh オブジェクトが持つ subsets メンバーへの参照を作成
		std::vector<mesh::subset>& subsets{ mesh.subsets };

		/*FBX SDKの機能を使って、
		現在のFBXメッシュにいくつのマテリアルが割り当てられているかを取得*/
		const int material_count{ fbx_mesh->GetNode()->GetMaterialCount() };

		//subsets ベクターのサイズを、material_count に合わせて調整
		subsets.resize(material_count > 0 ? material_count : 1);

		//取得したマテリアルの数だけループ
		for (int material_index = 0;
			material_index < material_count;
			material_index++)
		{
			/*FBXメッシュに割り当てられている
			個々のFBXマテリアル（FbxSurfaceMaterial）を取得*/
			const FbxSurfaceMaterial* fbx_material

			{ fbx_mesh->GetNode()->GetMaterial(material_index) };

			/*FBXマテリアルの名前と一意なIDを、
			対応する mesh::subset オブジェクトに格納*/
			subsets.at(material_index).material_name = 
				fbx_material->GetName();
			subsets.at(material_index).material_unique_id =
				fbx_material->GetUniqueID();
		}

		for (const vertex& v : mesh.vertices)
		{
			mesh.bounding_box[0].x = std::min<float>(mesh.bounding_box[0].x, v.position.x);
			mesh.bounding_box[0].y = std::min<float>(mesh.bounding_box[0].y, v.position.y);
			mesh.bounding_box[0].z = std::min<float>(mesh.bounding_box[0].z, v.position.z);
			mesh.bounding_box[1].x = std::min<float>(mesh.bounding_box[1].x, v.position.x);
			mesh.bounding_box[1].y = std::min<float>(mesh.bounding_box[1].y, v.position.y);
			mesh.bounding_box[1].z = std::min<float>(mesh.bounding_box[1].z, v.position.z);

		}


		//各マテリアルが適用されるポリゴンのインデックス範囲を特定するための準備


		if (material_count > 0)
		{
			//現在のFBXメッシュが持つポリゴンの総数を取得
			const int polygon_count{ fbx_mesh->GetPolygonCount() };

			//すべてのポリゴンをループで処理
			for (int polygon_index = 0;
				polygon_index < polygon_count;
				polygon_index++)
			{
				/*FBX SDKの機能を使って、
				現在のポリゴンにどのマテリアル（のインデックス）
				が割り当てられているかを取得*/
				const int material_index
				{ fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(polygon_index) };

				/*1つのポリゴン（三角形）は3つのインデックスで構成されるため
				取得した material_index に対応する 
				subset オブジェクトの index_count を 3 ずつ増やす*/
				subsets.at(material_index).index_count += 3;
			}


			//インデックスオフセットの計算と index_count のリセット


			//インデックスバッファの開始オフセットを追跡するための変数
			uint32_t offset{ 0 };

			//計算された index_count を持つ各サブセットに対してループ
			for (mesh::subset& subset : subsets)
			{
				/*現在のサブセットのインデックスデータが、
				全体のインデックスバッファのどこから始まるべきかを示す
				オフセットを記録*/
				subset.start_index_location = offset;

				/*offset を次のサブセットの開始位置に更新するために、
				現在のサブセットの index_count を加算*/
				offset += subset.index_count;

				// This will be used as counter in the following procedures, reset to zero
				//index_count を 0 にリセット
				subset.index_count = 0;
			}
		}

		//メッシュにマテリアルIDを設定
		if (fbx_node->GetMaterialCount() > 0)
		{
			const FbxSurfaceMaterial* fbx_material = fbx_node->GetMaterial(0);
			mesh.material_id = fbx_material->GetUniqueID();
		}
		else
		{
			//ダミーマテリアルのIDを設定
			mesh.material_id = dummy_material.unique_id;
		}


		//頂点・インデックス配列のサイズ確保


		//FBXメッシュが持つポリゴン（三角形）の数を取得
		const int polygon_count{ fbx_mesh->GetPolygonCount() };

		/*必要な頂点数（ポリゴン数 × 3）に基づいて、
		mesh.verticesベクターのサイズを確保*/
		mesh.vertices.resize(polygon_count * 3LL);

		//インデックスデータもポリゴン数 × 3のサイズを確保
		mesh.indices.resize(polygon_count * 3LL);


		//UVセット名の取得とコントロールポイントの取得


		FbxStringList uv_names;
		//テクスチャ座標（UV）のセット名を取得
		fbx_mesh->GetUVSetNames(uv_names);
		//FBXメッシュの「コントロールポイント」を取得
		const FbxVector4* control_points{ fbx_mesh->GetControlPoints() };


		//各ポリゴンと頂点のループ処理


		//FBXメッシュ内の各ポリゴン（三角形）を順番に処理
		for (int polygon_index = 0;
			polygon_index < polygon_count;
			polygon_index++)
		{
			/*現在処理しているポリゴン（polygon_indexで示されるポリゴン）に
			どのマテリアルが適用されているか を判断*/
			const int material_index{ material_count > 0 ?
			fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(polygon_index) : 0 };

			/*material_index を使用して、
			subsets ベクターから対応する 
			mesh::subset オブジェクトへの参照を取得*/
			mesh::subset& subset{ subsets.at(material_index) };

			/*現在処理中のポリゴンのインデックスを、
			このサブセット内のインデックスバッファの
			どこに配置すべきかを示す「オフセット」を計算*/
			const uint32_t offset
			{ subset.start_index_location + subset.index_count };

			/*現在のポリゴンを構成する3つの頂点（position_in_polygonは0, 1, 2）
			を処理*/
			for (int position_in_polygon = 0; position_in_polygon < 3; position_in_polygon++)
			{
				//現在の頂点のグローバルなインデックスを計算
				const int vertex_index{ polygon_index * 3 + position_in_polygon };


				//頂点位置の取得と設定


				vertex vertex;
				/*特定のポリゴン内の特定の頂点が、
				どのコントロールポイントに対応するかを示すインデックスを取得*/
				const int polygon_vertex{ fbx_mesh->GetPolygonVertex(polygon_index,position_in_polygon) };

				/*実際のコントロールポイントの座標（FbxVector4型）を取得し、
				vertex.positon（DirectX::XMFLOAT3型）に変換して格納*/
				vertex.position.x = static_cast<float>(control_points[polygon_vertex][0]);
				vertex.position.y = static_cast<float>(control_points[polygon_vertex][1]);
				vertex.position.z = static_cast<float>(control_points[polygon_vertex][2]);

				/*現在処理している頂点（polygon_vertexで示される頂点）
				が影響を受けるすべてのボーンのリストを取得*/
				const bone_influence_per_control_point& influences_per_control_point
				{ bone_influecnes.at(polygon_vertex) };

				/*現在の頂点に影響を与えるボーンのリスト」に含まれる
				各ボーンの影響情報について、一つずつループで処理*/
				for (size_t influence_index = 0;
					influence_index < influences_per_control_point.size();
					influence_index++)
				{
					/*ボーンの影響の数がMAX_BONE_INFLUENCESよりも少ない場合*/
					if (influence_index < MAX_BONE_INFLUENCES)
					{
						/*在の頂点が持つbone_weights配列の
						influence_index番目の要素に、
						このボーンの影響の重みを格納*/
						vertex.bone_weights[influence_index] =
							influences_per_control_point.at
							(influence_index).bone_weight;

						/*現在の頂点が持つbone_indices配列の
						influence_index番目の要素に、
						このボーンのインデックスを格納*/
						vertex.bone_indices[influence_index] =
							influences_per_control_point.at
							(influence_index).bone_index;
					}
				}


				//法線ベクトルの取得と設定


				if (fbx_mesh->GetElementNormalCount() > 0)
				{
					FbxVector4 normal;
					/*特定のポリゴン内の特定の頂点の法線ベクトルを取得し、
					vertex.normalに変換して格納*/
					fbx_mesh->GetPolygonVertexNormal(polygon_index, position_in_polygon, normal);
					vertex.normal.x = static_cast<float>(normal[0]);
					vertex.normal.y = static_cast<float>(normal[1]);
					vertex.normal.z = static_cast<float>(normal[2]);
				}


				//テクスチャ座標（UV）の取得と設定


				//UV情報があるかを確認
				if (fbx_mesh->GetElementUVCount() > 0)
				{
					FbxVector2 uv;
					bool unmapped_uv;
					//特定のポリゴン内の特定の頂点のUV座標を取得
					fbx_mesh->GetPolygonVertexUV(
						polygon_index,
						position_in_polygon,
						uv_names[0],
						uv,
						unmapped_uv);
					vertex.texcoord.x = static_cast<float>(uv[0]);

					//テクスチャが上下逆さまに貼られるのを防ぐ
					vertex.texcoord.y = 1.0f - static_cast<float>(uv[1]);
				}

				//FBXファイルから読み込んだメッシュの各頂点に接線ベクトルを設定


				//FBX SDK に tangent（接線）データの生成を依頼する処理
				if (fbx_mesh->GenerateTangentsData(0, false))
				{
					//FBXメッシュから接線データを取得
					const FbxGeometryElementTangent* tangent = fbx_mesh->GetElementTangent(0);

					//接線ベクトルの値を1つの頂点に代入
					vertex.tangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(vertex_index)[0]);
					vertex.tangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(vertex_index)[1]);
					vertex.tangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(vertex_index)[2]);
					vertex.tangent.w = static_cast<float>(tangent->GetDirectArray().GetAt(vertex_index)[3]);
				}

				//頂点とインデックスの格納


				//mesh.verticesベクターの計算されたvertex_indexの位置に格納
				mesh.vertices.at(vertex_index) = std::move(vertex);
				//各頂点を個別に格納

				/*現在処理しているサブセット（マテリアル部分）の
				インデックスデータが、
				mesh.indices 全体の中でどこから始まるべきか を示す*/
				mesh.indices.at(static_cast<size_t>(offset) + position_in_polygon) = vertex_index;

				/*subset（マテリアル部分）に、
				これまでにいくつのインデックスが追加されたか 
				をカウントするための変数*/
				subset.index_count++;
			}
		}
	}
}

void skinned_mesh::fetch_materials(
	FbxScene* fbx_scene, 
	std::unordered_map<uint64_t, material>& materials)
{


	//FBXシーン内の各ノード（オブジェクト）を順番に処理


	//既に読み込まれているシーンのノードの総数を取得
	const size_t node_count{ scene_view.nodes.size() };
	//取得したノードの総数分だけループを回し、各ノードを一つずつ処理
	for (size_t node_index = 0; node_index < node_count; node_index++)
	{


		/*各ノードから対応するFBXノードを取得し、
		そのノードに割り当てられているマテリアルの数を調べる*/


		//ノード情報を、プログラム内で定義した scene::node 構造体として取得
		const scene::node& node{ scene_view.nodes.at(node_index) };

		/*取得したノードの名前 (node.name) を使って、
		元のFBXシーン (fbx_scene) から、
		その名前を持つFBXのノード（FbxNode）を探して取得*/
		const FbxNode* fbx_node{ fbx_scene->FindNodeByName(node.name.c_str()) };

		/*FBXノードにいくつのマテリアルが割り当てられているか
		（例えば、モデルの胴体部分と腕部分で
		異なるマテリアルが使われている場合など）を調べ、その数を取得*/
		const int material_count{ fbx_node->GetMaterialCount() };


		/*ノードに割り当てられている各マテリアルの情報を抽出し、
		プログラム内のmaterial構造体に格納*/


		/*現在のノードに割り当てられているマテリアルの数だけループを回し、
		各マテリアルを一つずつ処理*/
		for (int material_index = 0; material_index < material_count; material_index++)
		{
			/*現在のループで処理しているマテリアルを、
			FBX SDKが提供する FbxSurfaceMaterial 型で取得*/
			const FbxSurfaceMaterial* fbx_material{ fbx_node->GetMaterial(material_index) };

			/*プログラム内で定義した skinned_mesh::material 型の
			新しいインスタンスを作成*/
			material material;

			/*FBXマテリアルの名前を取得し、material 構造体の nama メンバーに設定*/
			material.name = fbx_material->GetName();

			/*FBXマテリアルのユニークIDを取得し、
			material 構造体の uinque_id メンバーに設定*/
			material.unique_id = fbx_material->GetUniqueID();

			/*FBXマテリアルから特定のプロパティ（特性）を取得するための変数*/
			FbxProperty fbx_property;

			/*現在のマテリアルから「拡散色（Diffuse Color）」のプロパティを探す*/
			fbx_property = fbx_material->FindProperty(FbxSurfaceMaterial::sDiffuse);

			/*拡散色のプロパティが有効（存在）するかどうかを確認*/
			if (fbx_property.IsValid())
			{
				/*拡散色の値（RGBの3つの倍精度浮動小数点数）を取得*/
				const FbxDouble3 color{ fbx_property.Get<FbxDouble3>() };

				/*取得した拡散色のR, G, B値を、
				プログラム内の material.kd (拡散光の色) に
				浮動小数点数としてコピー*/
				material.kd.x = static_cast<float>(color[0]);
				material.kd.y = static_cast<float>(color[1]);
				material.kd.z = static_cast<float>(color[2]);
				material.kd.w = 1.0f;

				/*拡散色プロパティにテクスチャが関連付けられている場合、
				そのテクスチャ情報を取得*/
				const FbxFileTexture* fbx_texture{ fbx_property.GetSrcObject<FbxFileTexture>() };

				/*テクスチャが見つかった場合、
				そのテクスチャの相対ファイル名を
				material.texture_filenames 配列の最初の要素に格納*/
				material.texture_filenames[0] =
					fbx_texture ? fbx_texture->GetRelativeFileName() : "";
			}

			//このマテリアルに法線マップが設定されているか？を探す
			fbx_property = fbx_material->FindProperty(FbxSurfaceMaterial::sNormalMap);

			//法線マップのプロパティが存在するか確認
			if (fbx_property.IsValid())
			{
				//法線マップに関連付けられているテクスチャファイル（画像）を取得
				const FbxFileTexture* file_texture{ fbx_property.GetSrcObject<FbxFileTexture>() };

				//テクスチャファイルが見つかったら、そのファイル名を文字列として保存
				material.texture_filenames[1] = file_texture ? file_texture->GetRelativeFileName() : "";
			}

			/*作成した material インスタンスを、
			ユニークIDをキーとして materials マップに追加*/
			materials.emplace(material.unique_id, std::move(material));
		}
	}
	//全てのノードのマテリアルを読み込んだ後、ダミーマテリアルをマップに追加
	materials.emplace();
}

/// <summary>
/// バッファの生成
/// </summary>
/// <param name="device">デバイス</param>
/// <param name="fbx_filename">fbx ファイル名</param>
void skinned_mesh::create_com_objects(ID3D11Device* device, const char* fbx_filename)
{
	for (mesh& mesh : meshes)
	{
		HRESULT hr{ S_OK };
		D3D11_BUFFER_DESC buffer_desc{};
		D3D11_SUBRESOURCE_DATA subresource_data{};
		buffer_desc.ByteWidth = static_cast<UINT>(sizeof(vertex) * mesh.vertices.size());
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		subresource_data.pSysMem = mesh.vertices.data();
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;
		hr = device->CreateBuffer(&buffer_desc, &subresource_data,
			mesh.vertex_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		subresource_data.pSysMem = mesh.indices.data();
		hr = device->CreateBuffer(&buffer_desc, &subresource_data,
			mesh.index_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
#if 1
		mesh.vertices.clear();
		mesh.indices.clear();
#endif
	}

	// マテリアルの中にあるテクスチャファイル名の数だけテクスチャを生成する
	for (std::unordered_map<uint64_t, material>::iterator iterator = materials.begin();
		iterator != materials.end(); iterator++)
	{
		//拡散マップ（0番）と法線マップ（1番）の2つを対象
		for (size_t texture_index = 0; texture_index < 2; texture_index++)
		{
			//テクスチャファイル名が指定されていれば
			if (iterator->second.texture_filenames[texture_index].size() > 0)
			{
				//FBXファイルのパスをベースに、テクスチャファイルのパスを構築
				std::filesystem::path path(fbx_filename);
				path.replace_filename(iterator->second.texture_filenames[texture_index]);
				D3D11_TEXTURE2D_DESC texture2d_desc;

				//実際の画像ファイルを読み込んで GPU リソースを作成
				load_texture_from_file(device, path.c_str(),
					iterator->second.shader_resource_views[texture_index].GetAddressOf(), &texture2d_desc);
			}
			else
			{
				// ダミーテクスチャの生成
				make_dummy_texture(device, iterator->second.shader_resource_views[texture_index].GetAddressOf(),
					texture_index == 1 ? 0xFFFF7F7F : 0xFFFFFFFF, 16);
			}
		}
	}

	HRESULT hr = S_OK;
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0,
			D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_vs_from_cso(device, "skinned_mesh_vs.cso", vertex_shader.ReleaseAndGetAddressOf(),
		input_layout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, "skinned_mesh_ps.cso", pixel_shader.ReleaseAndGetAddressOf());

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}




//描画処理
void skinned_mesh::render(
	ID3D11DeviceContext* immediate_context,
	const DirectX::XMFLOAT4X4& world,
	const DirectX::XMFLOAT4& material_color,
	const animation::keyframe* keyframe)
{
	for (const mesh& mesh : meshes)
	{


		//頂点バッファとインデックスバッファの設定


		uint32_t stride{ sizeof(vertex) };
		uint32_t offset{ 0 };
		// GPUに対して、描画するモデルの「形」が格納されている頂点バッファを設定
		immediate_context->IASetVertexBuffers(
			0, 1,
			mesh.vertex_buffer.GetAddressOf(),
			&stride,	//1つの頂点データが何バイトか（sizeof(vertex)）を指定
			&offset);	//頂点バッファの開始位置を（通常は0）を指定

		//GPUに対して、頂点バッファからどの頂点を使ってポリゴン（面）を形成するかを指定するインデックスバッファを設定
		immediate_context->IASetIndexBuffer(
			mesh.index_buffer.Get(),
			DXGI_FORMAT_R32_UINT,	//インデックスが32ビット符号なし整数形式で設定
			0);


		//プリミティブトポロジーの設定


		//描画するデータの「形の種類」を指定
		immediate_context->IASetPrimitiveTopology(
			/*「渡された頂点とインデックスを使って、
			独立した三角形のリストとして描画*/
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		);

		//GPUに渡す頂点データの形式をD3D(Direct3D)に教える
		immediate_context->IASetInputLayout(input_layout.Get());


		//シェーダーの設定


		//GPUの頂点シェーダーを設定
		immediate_context->VSSetShader(
			vertex_shader.Get(),
			nullptr, 0
		);
		//GPUのピクセルシェーダーを設定
		immediate_context->PSSetShader(
			pixel_shader.Get(),
			nullptr, 0
		);

		////マテリアルが存在するかの確認
		//if (!materials.empty())
		//{
		//	/*ピクセルシェーダーにテクスチャを設定*/
		//	immediate_context->PSSetShaderResources(
		//		0, 1,
		//		materials.cbegin()->second.shader_resource_views[0].GetAddressOf()
		//	);
		//}
		//else
		//{
		//	//
		//}

		//定数バッファの更新と設定


		//world行列とmaterial_colorを保持
		constants data;
		//data.world = world;

		//XMStoreFloat4x4(&data.world,XMLoadFloat4x4(&mesh.default_global_transform) * XMLoadFloat4x4(&world));
		
		//アニメーションがあるかどうか
		if (keyframe && keyframe->nodes.size() > 0)
		{
			const animation::keyframe::node& mesh_node{ keyframe->nodes.at(mesh.node_index) };
			XMStoreFloat4x4(&data.world, XMLoadFloat4x4(&mesh_node.global_transform) * XMLoadFloat4x4(&world));

			const size_t bone_count{ mesh.bind_pose.bones.size() };
			_ASSERT_EXPR(bone_count < MAX_BONES, L"The value of the 'bone_count' has exceeded MAX_BONES.");

			//今のインデックスに対応するボーンを取得
			for (size_t bone_index = 0; bone_index < bone_count; bone_index++)
			{
				const skeleton::bone& bone{ mesh.bind_pose.bones.at(bone_index) };
				const animation::keyframe::node& bone_node{ keyframe->nodes.at(bone.node_index) };

				//最終的なボーン変形行列を計算し、data.bone_transforms[] に格納
				XMStoreFloat4x4(&data.bone_transforms[bone_index],
					XMLoadFloat4x4(&bone.offset_transform) *
					XMLoadFloat4x4(&bone_node.global_transform) *
					//XMMatrixInverse(nullptr, XMLoadFloat4x4(&mesh.default_global_transform))
					XMMatrixInverse(nullptr, XMLoadFloat4x4(&mesh_node.global_transform))
				);
			}
		}
		else
		{
			//ワールド行列をセット
			XMStoreFloat4x4(&data.world,
				XMLoadFloat4x4(&mesh.default_global_transform) * XMLoadFloat4x4(&world));
			for (size_t bone_index = 0; bone_index < MAX_BONES; bone_index++)
			{
				//ボーン変換行列を初期化
				data.bone_transforms[bone_index] = { 
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1 };
			}
		}
#if 0
		//何も回転・移動・スケーリングしない
		DirectX::XMStoreFloat4x4(&data.bone_transforms[0], XMMatrixIdentity());

		//ボーン1をZ軸まわりに +45度回転
		DirectX::XMStoreFloat4x4(&data.bone_transforms[1], DirectX::XMMatrixRotationRollPitchYaw(
			0, 0, DirectX::XMConvertToRadians(45)));

		//ボーン2にZ軸 -45度回転
		DirectX::XMStoreFloat4x4(&data.bone_transforms[2], DirectX::XMMatrixRotationRollPitchYaw(
			0, 0, DirectX::XMConvertToRadians(45)));

#endif

#if 0
		//ボーンの「逆バインドポーズ行列」
		DirectX::XMMATRIX B[3];

		//各ボーンに格納されている「逆バインド行列」を読み込みます
		B[0] = XMLoadFloat4x4(&mesh.bind_pose.bones.at(0).offset_transform); 
		B[1] = XMLoadFloat4x4(&mesh.bind_pose.bones.at(1).offset_transform); 
		B[2] = XMLoadFloat4x4(&mesh.bind_pose.bones.at(2).offset_transform); 

		//現在のボーンのアニメーション行列
		DirectX::XMMATRIX A[3];

		//各ボーンの現在の姿勢を表すアニメーション行列
		A[0] = XMMatrixRotationRollPitchYaw(XMConvertToRadians(90), 0, 0);//X軸に90度回転
		A[1] = XMMatrixRotationRollPitchYaw(0, 0, XMConvertToRadians(45)) * XMMatrixTranslation(0, 2, 0);//Z軸に45度 + 上に移動
		A[2] = XMMatrixRotationRollPitchYaw(0, 0, XMConvertToRadians(-45)) * XMMatrixTranslation(0, 2, 0);//Z軸に-45度 + 上に移動

		//合成して最終ボーン変換を作成
		XMStoreFloat4x4(&data.bone_transforms[0], B[0] * A[0]);
		XMStoreFloat4x4(&data.bone_transforms[1], B[1] * A[1] * A[0]);
		XMStoreFloat4x4(&data.bone_transforms[2], B[2] * A[2] * A[1] * A[0]);
#endif
		//複数のマテリアルを持つメッシュを、
		// そのマテリアルごとに異なる部分（サブセット）に分けて描画


		for (const mesh::subset& subset : mesh.subsets)
		{
			//サブセットに割り当てられたマテリアルを取得
			const material* material = &materials.at(subset.material_unique_id);

#if false
			auto it = materials.find(subset.material_unique_id);

			if (it != materials.end())
			{
				//マテリアルが見つかった場合はそれを使用
				material = &it->second;
			}
			else
			{
				// マテリアルが見つからなかった場合はダミーマテリアルを使用
				material = &dummy_material;
			}
#endif
			/*オブジェクトの最終的なマテリアルカラーを計算し、
			GPUに送るデータ (data.material_color) に設定*/
			XMStoreFloat4(&data.material_color,
				XMLoadFloat4(&material_color) * XMLoadFloat4(&material->kd));



			//constant_buffer（GPU上の定数バッファ）のデータを、
			// CPU側のdataで更新
			immediate_context->UpdateSubresource(
				constant_buffer.Get(),
				0, 0,
				&data,
				0, 0
			);
			//更新された定数バッファを、
			// 頂点シェーダーがアクセスできるように設定
			immediate_context->VSSetConstantBuffers(
				0, 1,
				constant_buffer.GetAddressOf()
			);

			//テクスチャの設定
			immediate_context->PSSetShaderResources(
				0, 1,
				material->shader_resource_views[0].GetAddressOf()
			);


			immediate_context->PSSetShaderResources(
				1, 1,
				material->shader_resource_views[1].GetAddressOf()
			);

			//サブセットの描画命令
			immediate_context->DrawIndexed(
				subset.index_count,
				subset.start_index_location,
				0
			);
		}
	}
}

//FBXモデルのスケルトン構造を読み取り、bind_pose にボーンとして格納
void skinned_mesh::fetch_skeleton(FbxMesh* fbx_mesh, skeleton& bind_pose)
{
	//スキニング情報（=ボーンによる変形）」の数を取得
	const int deformer_count = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);

	//各スキニング情報（FbxSkin）に対して処理を行うループ
	for (int deformer_index = 0; deformer_index < deformer_count; deformer_index++)
	{
		//「このメッシュがどのボーンに影響されるか」を示す構造
		FbxSkin* skin = static_cast<FbxSkin*>(fbx_mesh->GetDeformer(deformer_index, FbxDeformer::eSkin));
		
		//各 skin には 複数のボーン（Cluster） が含まれており、その数を取得
		const int cluster_count = skin->GetClusterCount();

		//bind_pose にクラスタ（=ボーン）分だけのスロットを確保
		bind_pose.bones.resize(cluster_count);

		// 各ボーンに対して処理を行うループ
		for (int cluster_index = 0; cluster_index < cluster_count; cluster_index++)
		{
			//ボーン情報そのもの（FbxCluster）を取得
			FbxCluster* cluster = skin->GetCluster(cluster_index);

			//bind_pose.bones にある該当のボーン構造体を参照
			skeleton::bone& bone{ bind_pose.bones.at(cluster_index) };

			//ボーンの 名前 と 一意なID を取得
			bone.name = cluster->GetLink()->GetName();
			bone.unique_id = cluster->GetLink()->GetUniqueID();

			//親ノードのIDを使って、親のインデックスを取得
			bone.parent_index = bind_pose.indexof(cluster->GetLink()->GetParent()->GetUniqueID());
			
			// このボーンが scene::nodes 内でどのインデックスにあるかを記録
			bone.node_index = scene_view.indexof(bone.unique_id);

			//メッシュのバインドポーズ時のグローバル行列を取得(モデル側の初期姿勢{Tポーズ})
			FbxAMatrix reference_global_init_position;
			cluster->GetTransformMatrix(reference_global_init_position);

			// ボーン（クラスタ）側のバインドポーズのグローバル行列を取得
			FbxAMatrix cluster_global_init_position;
			cluster->GetTransformLinkMatrix(cluster_global_init_position);

			//offset_transform に 逆バインド行列 を格納
			bone.offset_transform = to_xmfloat4x4(
				cluster_global_init_position.Inverse() * reference_global_init_position
			);
		}
	}
}

//FBXファイルからアニメーションデータ（ボーンの動き）を読み取って、animation_clips に保存する処理
void skinned_mesh::fetch_animations(
	FbxScene* fbx_scene,
	std::vector<animation>& animation_clips,
	float sampling_rate)
{
	FbxArray<FbxString*> animation_stack_names;

	//アニメーションスタック（アニメーションクリップ）の名前を全部取得
	fbx_scene->FillAnimStackNameArray(animation_stack_names);
	const int animation_stack_count{ animation_stack_names.GetCount() };

	//各アニメーションスタックに対しての処理
	for (int animation_stack_index = 0; animation_stack_index < animation_stack_count; animation_stack_index++)
	{
		//新しいアニメーションクリップ（例：Walk）を1つ追加
		animation& animation_clip{ animation_clips.emplace_back() };
		animation_clip.name = animation_stack_names[animation_stack_index]->Buffer();

		//今処理しているアニメーションスタック（Walkなど）を有効にする
		FbxAnimStack* animation_stack{ fbx_scene->FindMember<FbxAnimStack>(animation_clip.name.c_str()) };
		fbx_scene->SetCurrentAnimationStack(animation_stack);
		const FbxTime::EMode time_mode{ fbx_scene->GetGlobalSettings().GetTimeMode() };
		FbxTime one_second;
		one_second.SetTime(0, 0, 1, 0, 0, time_mode);

		//サンプリング間隔を設定
		animation_clip.sampling_rate = sampling_rate > 0 ?
			sampling_rate : static_cast<float>(one_second.GetFrameRate(time_mode));
		const FbxTime sampling_interval
		{ static_cast<FbxLongLong>(one_second.Get() / animation_clip.sampling_rate) };
		const FbxTakeInfo* take_info{ fbx_scene->GetTakeInfo(animation_clip.name.c_str()) };
		
		//アニメーションの時間範囲を取得
		const FbxTime start_time{ take_info->mLocalTimeSpan.GetStart() };
		const FbxTime stop_time{ take_info->mLocalTimeSpan.GetStop() };

		//各フレーム時刻でボーンの姿勢を取得
		for (FbxTime time = start_time; time < stop_time; time += sampling_interval)
		{
			//フレームに対応する keyframe を1つ作成
			animation::keyframe& keyframe{ animation_clip.sequence.emplace_back() };

			const size_t node_count{ scene_view.nodes.size() };
			keyframe.nodes.resize(node_count);

			//cene_view.nodes（すべてのノード＝ボーン）に対して処理
			for (size_t node_index = 0; node_index < node_count; node_index++)
			{
				//FBXノード（ボーン）を名前で取得
				FbxNode* fbx_node{ fbx_scene->FindNodeByName(scene_view.nodes.at(node_index).name.c_str()) };
				if (fbx_node)
				{
					//指定時刻のアニメーションデータを取得して保存
					animation::keyframe::node& node{ keyframe.nodes.at(node_index) };

					//この時刻におけるグローバルトランスフォームを取得して保存
					node.global_transform = to_xmfloat4x4(fbx_node->EvaluateGlobalTransform(time));
				
					//ローカル変換を取得して、スケール、回転、平行移動に分解して保存
					const FbxAMatrix& local_transform{ fbx_node->EvaluateLocalTransform(time) };
					node.scaling = to_xmfloat3(local_transform.GetS());
					node.rotation = to_xmfloat4(local_transform.GetQ());
					node.translation = to_xmfloat3(local_transform.GetT());
				}
			}
		}
	}
	//後始末（アニメーションクリップ名文字列の解放）
	for (int animation_stack_index = 0; animation_stack_index < animation_stack_count; animation_stack_index++)
	{
		delete animation_stack_names[animation_stack_index];
	}
}

//アニメーション更新
void skinned_mesh::update_animation(animation::keyframe& keyframe)
{
	//現在のアニメーションキーフレームに含まれるノードの数を取得
	size_t node_count{ keyframe.nodes.size() };

	//ノード（ボーン）を1つずつ処理
	for (size_t node_index = 0; node_index < node_count; node_index++)
	{
		animation::keyframe::node& node{ keyframe.nodes.at(node_index) };
		XMMATRIX S{ XMMatrixScaling(node.scaling.x,node.scaling.y,node.scaling.z) };//スケーリング行列
		XMMATRIX R{ XMMatrixRotationQuaternion(XMLoadFloat4(&node.rotation)) };//回転行列
		XMMATRIX T{ XMMatrixTranslation(node.translation.x,node.translation.y,node.translation.z) };//平行移動行列

		//親のグローバル変換を取得
		int64_t parent_index{ scene_view.nodes.at(node_index).parent_index };
		XMMATRIX P{ parent_index < 0 ? XMMatrixIdentity() :
		XMLoadFloat4x4(&keyframe.nodes.at(parent_index).global_transform) };

		// グローバルトランスフォームを計算して保存
		XMStoreFloat4x4(&node.global_transform, S * R * T * P);
	}
}

//FBXファイルからアニメーションクリップを読み込み、skinned_mesh に追加する処理
bool skinned_mesh::append_animations(const char* animation_filename, float sampling_rate)
{
	//FBXマネージャとシーンを作成
	FbxManager* fbx_manager{ FbxManager::Create() };
	FbxScene* fbx_scene{ FbxScene::Create(fbx_manager,"") };

	//インポーター作成してファイルを読み込む
	FbxImporter* fbx_importer{ FbxImporter::Create(fbx_manager,"") };
	bool import_status{ false };
	import_status = fbx_importer->Initialize(animation_filename);	//アニメーションFBXを読み込み準備
	_ASSERT_EXPR_A(import_status, fbx_importer->GetStatus().GetErrorString());
	import_status = fbx_importer->Import(fbx_scene);//実際にFBXファイルのデータを fbx_scene に読み込む
	_ASSERT_EXPR_A(import_status, fbx_importer->GetStatus().GetErrorString());

	if (!import_status)
	{
		fbx_manager->Destroy();
		return false;
	}

	//アニメーションクリップを取得
	fetch_animations(fbx_scene, animation_clips, sampling_rate);

	//リソースを解放
	fbx_manager->Destroy();

	//成功を返す
	return true;
}

void skinned_mesh::blend_animations
(
	const animation::keyframe* keyframes[2],//入力する2つのキーフレーム
	float factor,//補間係数 (0.0〜1.0)
	animation::keyframe& keyframe//出力先（補間結果をここに書き込む）
)
{
	//ノード数を取得して出力先を準備
	size_t node_count{ keyframes[0]->nodes.size() };
	keyframe.nodes.resize(node_count);

	// 各ノード（ボーン）ごとに補間
	for (size_t node_index = 0; node_index < node_count; node_index++)
	{
		XMVECTOR S[2]{
			XMLoadFloat3(&keyframes[0]->nodes.at(node_index).scaling),
			XMLoadFloat3(&keyframes[1]->nodes.at(node_index).scaling)
		};
		//スケーリングの線形補間
		XMStoreFloat3(&keyframe.nodes.at(node_index).scaling, XMVectorLerp(S[0], S[1], factor));

		XMVECTOR R[2]{
			XMLoadFloat4(&keyframes[0]->nodes.at(node_index).rotation),
			XMLoadFloat4(&keyframes[1]->nodes.at(node_index).rotation),
		};
		//回転（クォータニオン）の球面線形補間
		XMStoreFloat4(&keyframe.nodes.at(node_index).rotation, XMQuaternionSlerp(R[0], R[1], factor));

		XMVECTOR T[2]{
			XMLoadFloat3(&keyframes[0]->nodes.at(node_index).translation),
			XMLoadFloat3(&keyframes[1]->nodes.at(node_index).translation),
		};
		//平行移動の線形補間
		XMStoreFloat3(&keyframe.nodes.at(node_index).translation, XMVectorLerp(T[0], T[1], factor));
	}
}
