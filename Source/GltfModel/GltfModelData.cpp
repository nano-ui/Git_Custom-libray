#include "GltfModelData.h"
#include "../Serialization/GltfModelSerializer.h"
#include <stack>
#include <texture.h>
#include <misc.h>
#include <shader.h>

//==============================================
//画像読み込みをスキップするためのダミー関数
//==============================================
bool null_load_image_data(tinygltf::Image*, const int, std::string*, std::string*,
	int, int, const unsigned char*, int, void*)
{
	//常に成功を返して画像処理をバイパスする
	return true;
}

//デバイスとファイルを受け取ってモデルデータを初期化
GltfModelData::GltfModelData(const Microsoft::WRL::ComPtr<ID3D11Device>& device, const std::string& filename)
{
	this->filename = filename;	//ファイル名を保持

	//-----------------------
	//tinygltfのロード設定
	//-----------------------
	tinygltf::TinyGLTF tiny_gltf;	//tinygltfのローダーオブジェクト
	tiny_gltf.SetImageLoader(null_load_image_data, nullptr);	//画像読み込みをスキップ

	tinygltf::Model gltf_model;	//解析結果を格納するモデル
	std::string error, warning;	//警告やエラーメッセージを格納
	bool succeeded = false;		//読み込みが成功したかを示すグラフ

	//-----------------------------------------
	//ファイルの拡張子に応じた読み込み処理
	//-----------------------------------------
	if (filename.find(".glb") != std::string::npos)	//ファイルに.glbが含まれているかを確認
	{
		succeeded = tiny_gltf.LoadBinaryFromFile(&gltf_model, &error, &warning, filename.c_str());	//バイナリ形式をして読み込む
	}
	else if (filename.find(".gltf") != std::string::npos)	//ファイル名に.gltfが含まれているかを確認
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());	//テキスト形式として読み込む
	}

	//---------------------
	//読み込み結果の検証
	//---------------------
	_ASSERT_EXPR_A(warning.empty(), warning.c_str());	//警告があればデバッグ出力
	_ASSERT_EXPR_A(error.empty(), error.c_str());		//エラーがあればデバッグ出力し停止
	_ASSERT_EXPR_A(succeeded, L"failed to load gltf file");	//読み込み成功か確認

	//------------------
	//シーン情報の構築
	//------------------
	for (std::vector<tinygltf::Scene>::const_reference gltf_scene : gltf_model.scenes)	//モデル内の全シーンをループ
	{
		scene& scene = scenes.emplace_back();	//自身のシーンリストに新しい要素を追加
		scene.name = gltf_scene.name;			//シーン名をコピー
		scene.nodes = gltf_scene.nodes;			//シーンに所属するノードインデックスをコピー
	}
	default_scene = gltf_model.defaultScene;	//デフォルトシーンの番号を設定

	//------------------
	//モデル情報の解析
	//------------------
	FetchMeshes(gltf_model);			//メッシュデータの抽出とバッファの生成
	FetchNodes(gltf_model);				//ノード情報の抽出と階層行列の計算
	FetchMaterials(gltf_model);			//マテリアルデータの抽出
	FetchTextures(gltf_model);			//テクスチャ情報の抽出
	FetchAnimations(gltf_model);		//アニメーション情報を抽出
	MapAnimationNames(gltf_model);		//抽出したアニメーション名から検索用マップを構築

	//-------------------------------------
	//バッファの生データを丸ごと保存
	//--------------------------------------
	for (const tinygltf::Buffer& gltf_buffer : gltf_model.buffers)
	{
		raw_buffers.push_back(gltf_buffer.data);	//CPU側に生のバイト配列をコピーして保持
	}
	//----------------------------------------------------
	//抽出した生データをもとに、GPU用のバッファを生成
	//----------------------------------------------------
	if (device)
	{
		CreateGpuResources(device.Get());
	}
}

//========================================
//復元した生データからGPUリソースを構築
//========================================
void GltfModelData::CreateGpuResources(ID3D11Device* device)
{
	buffers.resize(raw_buffers.size());	//格納用ベクトルをリサイズ

	//---------------------------------------------
	//メッシュのバッファーリソースビューの生成
	//---------------------------------------------
	buffers.resize(raw_buffers.size());										// 生成するバッファの数を生データに合わせる
	for (size_t i = 0; i < raw_buffers.size(); i++)							// 保持している全ての生バッファをループ
	{
		if (raw_buffers.at(i).empty()) return;								//生データが空っぽの場合はバッファを作らずにスキップ
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = static_cast<UINT>(raw_buffers.at(i).size());// バイトサイズを生データから取得
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;	// 頂点・インデックス両用としてフラグを立てる

		D3D11_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pSysMem = raw_buffers.at(i).data();				// メモリのポインタを生データ配列の先頭に指定

		HRESULT hr = device->CreateBuffer(&buffer_desc, &subresource_data, buffers.at(i).ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));							// バッファ生成を実行
	}

	std::vector<material::cbuffer> material_data;	//シェーダーに送るデータのみを格納する配列
	for (std::vector<material>::const_reference material : materials)	//解析済みの全マテリアルを走査
	{
		material_data.emplace_back(material.data);	//データ構造体のみを抽出してリストに追加
	}

	//---------------------------------------------
	//マテリアルのシェーダーリソースビューを生成
	//---------------------------------------------
	HRESULT hr;	//DirectXの関数実行結果を格納する変数
	Microsoft::WRL::ComPtr<ID3D11Buffer> material_buffer;	//バッファ本体を保存
	if (!material_data.empty())
	{
		D3D11_BUFFER_DESC buffer_desc = {};		//バッファの特性を定義する設定構造体
		buffer_desc.ByteWidth = static_cast<UINT>(sizeof(material::cbuffer) * material_data.size());	//マテリアルの合計バイトサイズを指定
		buffer_desc.StructureByteStride = sizeof(material::cbuffer);	//1要素(マテリアル1つ分)のサイズを指定
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;	//GPUによる読み書き可能に設定
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;	//シェーダーからリソースとしてアクセス可能に設定
		buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;	//配列形式でアクセスする「構造化バッファ」作成

		D3D11_SUBRESOURCE_DATA subresource_data = {};	//作成と同時に書き込む初期データを定義
		subresource_data.pSysMem = material_data.data();	//CPU側のメモリにあるマテリアル配列の先頭アドレスを指定
		hr = device->CreateBuffer(&buffer_desc, &subresource_data, material_buffer.GetAddressOf());	//バッファ作成
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));	//バッファが作成されたかチェック

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};	//シェーダーからバッファを見るための窓口(ビュー)の設定
		shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;	//構造化バッファの場合はフォーマットをUNKONWNに設定
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;	//ビューの対象が通常のバッファであることを指定
		shader_resource_view_desc.Buffer.NumElements = static_cast<UINT>(material_data.size());	//配列の要素数(マテリアル数)を指定
		hr = device->CreateShaderResourceView(material_buffer.Get(), &shader_resource_view_desc, material_resource_view.GetAddressOf());	//シェーダーリソースビューを作成
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));	//シェーダーリソースビューが作成されたかチェック
	}

	//------------------------------------------------------
	//テクスチャのシェーダーリソースビュー（SRV）の生成
	//------------------------------------------------------
	for (const image& img : images)
	{
		ID3D11ShaderResourceView* shader_resource_view = nullptr;
		HRESULT hr = S_OK;

		if (!img.raw_image_data.empty()) // 生データが保持されている場合（メモリからの読み込み）
		{
			hr = load_texture_from_memory(device, img.raw_image_data.data(), img.raw_image_data.size(), &shader_resource_view);
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		else if (!img.uri.empty()) // 外部ファイルパスが指定されている場合
		{
			std::filesystem::path path = filename;
			std::wstring w_filename = path.parent_path().concat(L"/").wstring() + std::wstring(img.uri.begin(), img.uri.end());
			D3D11_TEXTURE2D_DESC texture2d_desc;
			hr = load_texture_from_file(device, w_filename.c_str(), &shader_resource_view, &texture2d_desc);
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}

		if (hr == S_OK && shader_resource_view != nullptr)
		{
			texture_resource_views.emplace_back();
			texture_resource_views.back().Attach(shader_resource_view);
		}
		else
		{
			// テクスチャの読み込みに失敗した場合でも、インデックスのズレを防ぐために空の要素を追加する
			texture_resource_views.emplace_back(nullptr);
		}
	}

}

//================================================
//バイナリキャッシュを利用してモデルを読み込む
//================================================
std::shared_ptr<GltfModelData> GltfModelData::Load(ID3D11Device* device, const std::string& filename)
{
	//-----------------------------
	//キャッシュファイル名の生成
	//-----------------------------
	std::string binary_filename = filename + ".bin";							//元の名前に「.bin」を付与してキャッシュ用ファイル名とする
	std::shared_ptr<GltfModelData> data = std::make_shared<GltfModelData>();	//空のデータインスタンスを生成

	//-------------------------------
	//キャッシュからのロード試行
	//-------------------------------
	if (GltfModelSerializer::Load(binary_filename, data))	//バイナリファイルが存在し、読み込みに成功した場合
	{
		data->CreateGpuResources(device);	//復元した生データから即座にGPUバッファを生成
		return data;						//完成したデータを返す
	}	

	//------------------------------------
	//キャッシュがない場合の通常読み込み
	//------------------------------------
	Microsoft::WRL::ComPtr<ID3D11Device> com_device(device);
	data = std::make_shared<GltfModelData>(com_device, filename);

	//------------------------------------
	//次回起動時のためにベイク保存処理
	//------------------------------------
	GltfModelSerializer::Save(binary_filename, data);

	return data;
}

//=================================================
//tinygltfの型情報をDirectXのフォーマットに変換
//=================================================
DXGI_FORMAT GltfModelData::ConvertFormat(const tinygltf::Accessor& accessor)
{
	switch (accessor.type)	//gltfの型(スカラー、ベクトル等)で分岐
	{
	case TINYGLTF_TYPE_SCALAR:	//スカラー値の場合
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			return DXGI_FORMAT_R8_UINT;	//8ビット符号あり整数
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return DXGI_FORMAT_R16_UINT;	//16ビット符号なし整数
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return DXGI_FORMAT_R32_UINT;	//32ビット符号無し整数
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported");
			return DXGI_FORMAT_UNKNOWN;	//未対応
		}
	case TINYGLTF_TYPE_VEC2:	//2次元ベクトルの場合
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;	//UV座標などで使用
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported");
			return DXGI_FORMAT_UNKNOWN;	//未対応
		}
	case TINYGLTF_TYPE_VEC3:	//３次元ベクトルの場合
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;	//座標、法線などで使用
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported");
			return DXGI_FORMAT_UNKNOWN;	//未対応
		}
	case TINYGLTF_TYPE_VEC4:	//4次元ベクトルの場合
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return DXGI_FORMAT_R8G8B8A8_UINT;	//色、ウェイトなど
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return DXGI_FORMAT_R16G16B16A16_UINT;	//色、ウェイトなど
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return DXGI_FORMAT_R32G32B32A32_UINT;	//色、ウェイトなど
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;	//色、ウェイトなど
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported");
			return DXGI_FORMAT_UNKNOWN;	//未対応
		}
		break;
	default:
		_ASSERT_EXPR(FALSE, L"This accessor component type is not supported");
		return DXGI_FORMAT_UNKNOWN;	//未知の型
	}
}

//=================================
//メッシュ情報とGPUバッファを生成
//=================================
void GltfModelData::FetchMeshes(const tinygltf::Model& gltf_model)
{
	//-------------------------------------
	//メッシュとプリミティブ情報の構成
	//-------------------------------------
	for (std::vector<tinygltf::Mesh>::const_reference gltf_mesh : gltf_model.meshes)	//全メッシュをループ
	{
		mesh& mesh = meshes.emplace_back();	//自身のリストにメッシュを追加
		mesh.name = gltf_mesh.name;	//メッシュ名をコピー
		for (std::vector<tinygltf::Primitive>::const_reference gltf_primitive : gltf_mesh.primitives)	//メッシュ内の全プリミティブをループ
		{
			mesh::primitive& primitive = mesh.primitives.emplace_back();	//自身のリストにプリミティブ追加
			primitive.material = gltf_primitive.material;	//マテリアル番号をコピー

			//------------------------------------
			//インデックスバッファビューの設定
			//------------------------------------
			if (gltf_primitive.indices > -1)	//インデックスバッファが存在するか場合
			{
				const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_primitive.indices);	//アクセサを取得
				const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//ビューを取得

				primitive.index_buffer_view.format = ConvertFormat(gltf_accessor);	//データ型をDXGI形式に変換
				primitive.index_buffer_view.buffer = gltf_buffer_view.buffer;		//参照バッファをコピー
				primitive.index_buffer_view.stride_in_bytes = gltf_accessor.ByteStride(gltf_buffer_view);	//ストライドを計算
				primitive.index_buffer_view.byte_offset = gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;	//合計オフセットを計算
				primitive.index_buffer_view.count = gltf_accessor.count;	//要素数を格納
			}

			//----------------------------
			//頂点バッファビューの設定
			//----------------------------
			for (std::map<std::string, int>::const_reference gltf_attribute : gltf_primitive.attributes)	//全頂点属性を走査
			{
				const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_attribute.second);	//属性ごとのアクセサを取得
				const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//ビューを取得

				buffer_view vertex_buffer_view = {};	//頂点バッファビュー情報を構築
				vertex_buffer_view.format = ConvertFormat(gltf_accessor);	//データ型をDXGI形式に変換
				vertex_buffer_view.buffer = gltf_buffer_view.buffer;	//バッファ番号をコピー
				vertex_buffer_view.stride_in_bytes = gltf_accessor.ByteStride(gltf_buffer_view);	//ストライドをコピー
				vertex_buffer_view.byte_offset = gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;	//開始位置を計算
				vertex_buffer_view.count = gltf_accessor.count;	//頂点数を保持

				primitive.vertex_buffer_views.emplace(std::make_pair(gltf_attribute.first, vertex_buffer_view));	//マップに登録
			}
		}
	}
}

//=========================================
//tinygltfのモデルからノード情報を抽出
//=========================================
void GltfModelData::FetchNodes(const tinygltf::Model& gltf_model)
{
	for (std::vector<tinygltf::Node>::const_reference gltf_node : gltf_model.nodes)	//モデルの全ノードを走査
	{
		node& node = nodes.emplace_back();	//自身のノードリストに新しい要素を追加
		node.name = gltf_node.name;			//ノード名を格納
		node.skin = gltf_node.skin;			//スキン番号を格納
		node.mesh = gltf_node.mesh;			//メッシュ番号を格納
		node.children = gltf_node.children;	//子ノードのリストをコピー

		//--------------------------------
		//行列またはTRSプロパティの抽出
		//--------------------------------
		if (!gltf_node.matrix.empty())	//ノードに行列が直接定義されている場合
		{
			DirectX::XMFLOAT4X4 matrix;	//一時的な行列格納変数
			for (size_t row = 0; row < MATRIX_DIMENSION; row++)	//行ループを行う
			{
				for (size_t column = 0; column < 4; column++)	//列ループを行う
				{
					matrix(row, column) = static_cast<float>(gltf_node.matrix.at(4 * row + column));	//行列要素に一つずつ転送
				}
			}

			DirectX::XMVECTOR S, R, T;	//スケール、回転、座標を格納
			bool succeed = DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&matrix));	//行列をTRS成分に分解
			_ASSERT_EXPR(succeed, L"Failed to decompose matrix");	//分解に失敗した際は停止

			DirectX::XMStoreFloat3(&node.scale, S);			//分解したスケールを格納
			DirectX::XMStoreFloat4(&node.rotation, R);		//分解した回転を格納
			DirectX::XMStoreFloat3(&node.translation, T);	//分解した座標を格納
		}
		else	//行列がなく、個別にスケール、回転、座標が定義されている場合
		{
			if (gltf_node.scale.size() > 0)	//スケール情報がある場合
			{
				node.scale.x = static_cast<float>(gltf_node.scale.at(0));	//Xスケールを設定
				node.scale.y = static_cast<float>(gltf_node.scale.at(1));	//Yスケールを設定
				node.scale.z = static_cast<float>(gltf_node.scale.at(2));	//Zスケールを設定
			}
			if (gltf_node.translation.size() > 0)	//座標情報がある場合
			{
				node.translation.x = static_cast<float>(gltf_node.translation.at(0));	//X座標を設定
				node.translation.y = static_cast<float>(gltf_node.translation.at(1));	//Y座標を設定
				node.translation.z = static_cast<float>(gltf_node.translation.at(2));	//Z座標を設定
			}
			if (gltf_node.rotation.size() > 0)	//回転情報がある場合
			{
				node.rotation.x = static_cast<float>(gltf_node.rotation.at(0));	//虚部Xを設定
				node.rotation.y = static_cast<float>(gltf_node.rotation.at(1));	//虚部Yを設定
				node.rotation.z = static_cast<float>(gltf_node.rotation.at(2));	//虚部Zを設定
				node.rotation.w = static_cast<float>(gltf_node.rotation.at(3));	//虚部Wを設定
			}
		}
	}
}

//===================================================
//tinygltfのモデルからマテリアルデータを抽出
//===================================================
void GltfModelData::FetchMaterials(const tinygltf::Model& gltf_model)
{
	//---------------------------------------
	//gltfモデル内の全マテリアルを走査
	//---------------------------------------
	for (std::vector<tinygltf::Material>::const_reference gltf_material : gltf_model.materials)
	{
		std::vector<material>::reference material = materials.emplace_back();	//マテリアル領域を追加

		material.name = gltf_material.name;	//マテリアル名を設定
		material.data.emissive_factor[0] = static_cast<float>(gltf_material.emissiveFactor.at(0));	//自己発光R成分を設定
		material.data.emissive_factor[1] = static_cast<float>(gltf_material.emissiveFactor.at(1));	//自己発光G成分を設定
		material.data.emissive_factor[2] = static_cast<float>(gltf_material.emissiveFactor.at(2));	//自己発光B成分を設定

		material.data.alpha_mode = gltf_material.alphaMode == "OPAQUE" ? 0 : gltf_material.alphaMode == "MASK" ? 1 : gltf_material.alphaMode == "BLEND" ? 2 : 0;
		material.data.alpha_cutoff = static_cast<float>(gltf_material.alphaCutoff);
		material.data.double_sided = gltf_material.doubleSided ? 1 : 0;

		material.data.pbr_metallic_roughness.basecolor_factor[0] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(0));	//ベースカラーのRを設定
		material.data.pbr_metallic_roughness.basecolor_factor[1] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(1));	//ベースカラーのGを設定
		material.data.pbr_metallic_roughness.basecolor_factor[2] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(2));	//ベースカラーのBを設定
		material.data.pbr_metallic_roughness.basecolor_factor[3] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(3));	//ベースカラーのAを設定
		material.data.pbr_metallic_roughness.basecolor_texture.index = gltf_material.pbrMetallicRoughness.baseColorTexture.index;	//ベースカラーテクスチャの参照番号を設定
		material.data.pbr_metallic_roughness.basecolor_texture.texcoord = gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord;	//ベースカラー用UV番号を設定
		material.data.pbr_metallic_roughness.metallic_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.metallicFactor);	//金属感係数を設定
		material.data.pbr_metallic_roughness.roughness_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.roughnessFactor);	//粗さ係数を設定
		material.data.pbr_metallic_roughness.metallic_roughness_texture.index = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index;	//金属・粗さテクスチャの番号を設定
		material.data.pbr_metallic_roughness.metallic_roughness_texture.texcoord = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;	//金属・粗さ用UV番号を設定

		material.data.normal_texture.index = gltf_material.normalTexture.index;	//法線マップの番号を設定
		material.data.normal_texture.texcoord = gltf_material.normalTexture.texCoord;	//法線マップのUV番号を設定
		material.data.normal_texture.scale = static_cast<float>(gltf_material.normalTexture.scale);	//法線の凹凸スケールを設定

		material.data.occlusion_texture.index = gltf_material.occlusionTexture.index;	//遮蔽マップの番号を設定
		material.data.occlusion_texture.texcoord = gltf_material.occlusionTexture.texCoord;	//遮蔽マップ用UV番号を設定
		material.data.occlusion_texture.strength = static_cast<float>(gltf_material.occlusionTexture.strength);	//遮蔽の強度を設定

		material.data.emissive_texture.index = gltf_material.emissiveTexture.index;			//発光テクスチャの番号を設定
		material.data.emissive_texture.texcoord = gltf_material.emissiveTexture.texCoord;	//発光テクスチャ用のUV番号を設定
	}
}

//=======================
//テクスチャ情報を抽出
//=======================
void GltfModelData::FetchTextures(const tinygltf::Model& gltf_model)
{
	HRESULT hr = S_OK;

	for (const tinygltf::Texture& gltf_texture : gltf_model.textures)
	{
		texture& texture = textures.emplace_back();
		texture.name = gltf_texture.name;
		texture.source = gltf_texture.source;
	}

	for (const tinygltf::Image& gltf_image : gltf_model.images)
	{
		image& image = images.emplace_back();
		image.name = gltf_image.name;
		image.width = gltf_image.width;
		image.height = gltf_image.height;
		image.component = gltf_image.component;
		image.bits = gltf_image.bits;
		image.pixel_type = gltf_image.pixel_type;
		image.buffer_view = gltf_image.bufferView;
		image.mime_type = gltf_image.mimeType;
		image.uri = gltf_image.uri;
		image.as_is = gltf_image.as_is;

		if (gltf_image.bufferView > -1)
		{
			const tinygltf::BufferView& buffer_view = gltf_model.bufferViews.at(gltf_image.bufferView);
			const tinygltf::Buffer& buffer = gltf_model.buffers.at(buffer_view.buffer);
			image.raw_image_data.assign(
				buffer.data.begin() + buffer_view.byteOffset,
				buffer.data.begin() + buffer_view.byteOffset + buffer_view.byteLength
			);
		}
	}
}

//===============================
//アニメーション情報を抽出
//===============================
void GltfModelData::FetchAnimations(const tinygltf::Model& gltf_model)
{
	using namespace std;
	using namespace tinygltf;
	using namespace DirectX;

	for (vector<Skin>::const_reference transmission_skin : gltf_model.skins)
	{
		skin& skin = skins.emplace_back();
		const Accessor& gltf_accessor = gltf_model.accessors.at(transmission_skin.inverseBindMatrices);
		const BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);
		skin.inverse_bind_matrices.resize(gltf_accessor.count);
		memcpy(
			skin.inverse_bind_matrices.data(),
			gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
			gltf_accessor.count * sizeof(XMFLOAT4X4));
		skin.joints = transmission_skin.joints;
	}

	for (vector<Animation>::const_reference gltf_animation : gltf_model.animations)
	{
		animation& animation = animations.emplace_back();
		animation.name = gltf_animation.name;
		for (vector<AnimationSampler>::const_reference gltf_sampler : gltf_animation.samplers)
		{
			animation::sampler& sampler = animation.samplers.emplace_back();
			sampler.input = gltf_sampler.input;
			sampler.output = gltf_sampler.output;
			sampler.interpolation = gltf_sampler.interpolation;

			const Accessor& gltf_accessor = gltf_model.accessors.at(gltf_sampler.input);
			const BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);
			const pair<unordered_map<int, vector<float>>::iterator, bool>& timelines{ animation.timelines.emplace(gltf_sampler.input, gltf_accessor.count) };
			if (timelines.second)
			{
				memcpy(
					timelines.first->second.data(),
					gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
					gltf_accessor.count * sizeof(FLOAT));
			}
		}
		for (vector<AnimationChannel>::const_reference gltf_channel : gltf_animation.channels)
		{
			animation::channel& channel = animation.channels.emplace_back();
			channel.sampler = gltf_channel.sampler;
			channel.target_node = gltf_channel.target_node;
			channel.target_path = gltf_channel.target_path;

			const AnimationSampler& gltf_sampler = gltf_animation.samplers.at(gltf_channel.sampler);
			const Accessor& gltf_accessor = gltf_model.accessors.at(gltf_sampler.output);
			const BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);

			if (gltf_channel.target_path == "scale")
			{
				const pair<unordered_map<int, vector<XMFLOAT3>>::iterator, bool>& scales = animation.scales.emplace(gltf_sampler.output, gltf_accessor.count);				if (scales.second)
				{
					memcpy(
						scales.first->second.data(),
						gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
						gltf_accessor.count * sizeof(XMFLOAT3));
				}
			}
			else if (gltf_channel.target_path == "rotation")
			{
				const pair<unordered_map<int, vector<XMFLOAT4>>::iterator, bool>& rotations = animation.rotations.emplace(gltf_sampler.output, gltf_accessor.count);
				if (rotations.second)
				{
					memcpy(
						rotations.first->second.data(),
						gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
						gltf_accessor.count * sizeof(XMFLOAT4));
				}
			}
			else if (gltf_channel.target_path == "translation")
			{
				const pair<unordered_map<int, vector<XMFLOAT3>>::iterator, bool>& translations = animation.translations.emplace(gltf_sampler.output, gltf_accessor.count);
				if (translations.second)
				{
					memcpy(
						translations.first->second.data(),
						gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
						gltf_accessor.count * sizeof(XMFLOAT3));
				}
			}
		}
	}
	for (decltype(animations)::reference animation : animations)
	{
		for (decltype(animation.timelines)::reference timelines : animation.timelines)
		{
			animation.duration = std::max<float>(animation.duration, timelines.second.back());
		}
	}
}

//===================================
//アニメーション名をマップに登録
//===================================
void GltfModelData::MapAnimationNames(const tinygltf::Model& gltf_model)
{
	//全てのアニメーションをループ
	for (size_t animation_index = 0; animation_index < animations.size(); animation_index++)
	{
		std::string animation_name = gltf_model.animations.at(animation_index).name;	//gltfデータからアニメーション名を取得
		if (animation_name.empty())	//モデラーが名前を設定しておらず空欄だった場合
		{
			animation_name = "anim_" + std::to_string(animation_index);	//anim_0, anim_1 のように連番の仮名を作成しエラーを防ぐ
		}
		animation_index_map.insert(std::make_pair(animation_name, animation_index));	//名前をキーとして、インデックス番号をマップに登録
	}
}
