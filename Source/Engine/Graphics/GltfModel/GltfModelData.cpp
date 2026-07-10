#include "GltfModelData.h"
#include "../Serialization/GltfModelSerializer.h"
#include "../Engine/Graphics/GpuResourceUtils.h"
#include <stack>
#include "../Engine/Graphics/texture.h"
#include "../Engine/Core/misc.h"
#include "../Engine\Graphics\Shaders\shader.h"


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
	this->filename = filename;

	tinygltf::TinyGLTF tiny_gltf;
	tiny_gltf.SetImageLoader(null_load_image_data, nullptr);

	tinygltf::Model gltf_model;
	std::string error, warning;
	bool succeeded = false;

	if (filename.find(".glb") != std::string::npos) //ファイル名に.glbが含まれているかの条件分岐
	{
		succeeded = tiny_gltf.LoadBinaryFromFile(&gltf_model, &error, &warning, filename.c_str());
	}
	else if (filename.find(".gltf") != std::string::npos) //ファイル名に.gltfが含まれているかの条件分岐
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
	}

	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, L"failed to load gltf file");

	for (std::vector<tinygltf::Scene>::const_reference gltf_scene : gltf_model.scenes) //全シーンを走査するループ
	{
		scene& scene = scenes.emplace_back();
		scene.name = gltf_scene.name;
		scene.nodes = gltf_scene.nodes;
	}
	default_scene = gltf_model.defaultScene;

	for (const tinygltf::Buffer& gltf_buffer : gltf_model.buffers) //全生バッファを走査するループ
	{
		raw_buffers.push_back(gltf_buffer.data);
	}

	FetchMeshes(gltf_model);
	FetchNodes(gltf_model);
	FetchMaterials(gltf_model);
	FetchTextures(gltf_model);
	FetchAnimations(gltf_model);
	MapAnimationNames(gltf_model);

	int nodes_with_mesh = 0; //メッシュを持つノードの総数カウンタ
	for (const auto& n : nodes) //全ノードを走査するループ
	{
		if (n.mesh > -1) //メッシュが割り当てられているかの条件分岐
		{
			nodes_with_mesh++;
		}
	}

	int total_primitives = 0; //全プリミティブの総数カウンタ
	for (const auto& m : meshes) //全メッシュを走査するループ
	{
		total_primitives += static_cast<int>(m.primitives.size());
	}

	char debug_buf[512]; //デバッグ文字列バッファ配列
	sprintf_s(debug_buf, "[GltfModelData Debug] File: %s | Nodes: %zu (With Mesh: %d) | Meshes: %zu (Primitives: %d)\n",
		this->filename.c_str(), nodes.size(), nodes_with_mesh, meshes.size(), total_primitives);
	OutputDebugStringA(debug_buf);

	if (device) //デバイスポインタが有効であるかの条件分岐
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
		if (raw_buffers.at(i).empty()) continue;								//生データが空っぽの場合はバッファを作らずにスキップ
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
	HRESULT hr;	//DirectXの関数実行結果を格納する
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
			hr = GpuResourceUtils::LoadTexture(device, img.raw_image_data.data(), img.raw_image_data.size(), &shader_resource_view);
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		else if (!img.uri.empty()) // 外部ファイルパスが指定されている場合
		{
			std::filesystem::path path = filename;
			std::string image_filename = path.parent_path().string() + "/" + img.uri;
			D3D11_TEXTURE2D_DESC texture2d_desc;
			hr = GpuResourceUtils::LoadTexture(device, image_filename.c_str(), &shader_resource_view, &texture2d_desc);
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
	switch (accessor.type)	//gltfの型(スカラー、ベクトル等)で判定
	{
	case TINYGLTF_TYPE_SCALAR:	//インデックスバッファ等
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:           return DXGI_FORMAT_R8_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return DXGI_FORMAT_R8_UINT;	//UEが容量節約によく使う型
		case TINYGLTF_COMPONENT_TYPE_SHORT:          return DXGI_FORMAT_R16_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return DXGI_FORMAT_R16_UINT;
		case TINYGLTF_COMPONENT_TYPE_INT:            return DXGI_FORMAT_R32_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return DXGI_FORMAT_R32_UINT;
		case TINYGLTF_COMPONENT_TYPE_FLOAT:          return DXGI_FORMAT_R32_FLOAT;
		default: break;
		}
		break;

	case TINYGLTF_TYPE_VEC2:	//UV座標等
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:           return accessor.normalized ? DXGI_FORMAT_R8G8_SNORM : DXGI_FORMAT_R8G8_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return accessor.normalized ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8G8_UINT;
		case TINYGLTF_COMPONENT_TYPE_SHORT:          return accessor.normalized ? DXGI_FORMAT_R16G16_SNORM : DXGI_FORMAT_R16G16_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return accessor.normalized ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R16G16_UINT;
		case TINYGLTF_COMPONENT_TYPE_INT:            return DXGI_FORMAT_R32G32_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return DXGI_FORMAT_R32G32_UINT;
		case TINYGLTF_COMPONENT_TYPE_FLOAT:          return DXGI_FORMAT_R32G32_FLOAT;
		default: break;
		}
		break;

	case TINYGLTF_TYPE_VEC3:	//座標、法線等
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:          return DXGI_FORMAT_R32G32B32_FLOAT;
		case TINYGLTF_COMPONENT_TYPE_INT:            return DXGI_FORMAT_R32G32B32_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return DXGI_FORMAT_R32G32B32_UINT;
		default: break;
		}
		break;

	case TINYGLTF_TYPE_VEC4:	//色、ジョイント、ウェイト等
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:           return accessor.normalized ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_R8G8B8A8_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return accessor.normalized ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UINT;
		case TINYGLTF_COMPONENT_TYPE_SHORT:          return accessor.normalized ? DXGI_FORMAT_R16G16B16A16_SNORM : DXGI_FORMAT_R16G16B16A16_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return accessor.normalized ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R16G16B16A16_UINT;
		case TINYGLTF_COMPONENT_TYPE_INT:            return DXGI_FORMAT_R32G32B32A32_SINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return DXGI_FORMAT_R32G32B32A32_UINT;
		case TINYGLTF_COMPONENT_TYPE_FLOAT:          return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default: break;
		}
		break;
	}

	//エラー原因特定のためのデバッグ出力を追加
	char debug_msg[256];
	sprintf_s(debug_msg, "[GltfModelData Error] Unsupported accessor! Type: %d, ComponentType: %d\n", accessor.type, accessor.componentType);
	OutputDebugStringA(debug_msg);

	_ASSERT_EXPR(FALSE, L"This accessor component type is not supported");
	return DXGI_FORMAT_UNKNOWN;	//未知の型
}

//=================================
//メッシュ情報とGPUバッファを生成
//=================================
void GltfModelData::FetchMeshes(const tinygltf::Model& gltf_model)
{
	for (std::vector<tinygltf::Mesh>::const_reference gltf_mesh : gltf_model.meshes)	//全メッシュをループ
	{
		mesh& mesh = meshes.emplace_back();	//自身のリストにメッシュを追加
		mesh.name = gltf_mesh.name;

		for (std::vector<tinygltf::Primitive>::const_reference gltf_primitive : gltf_mesh.primitives)	//メッシュ内の全プリミティブをループ
		{
			mesh::primitive& primitive = mesh.primitives.emplace_back();	//自身のリストにプリミティブ追加
			primitive.material = gltf_primitive.material;

			if (gltf_primitive.indices > -1)	//インデックスバッファが存在するか場合
			{
				const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_primitive.indices);	//アクセサを取得
				const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//ビューを取得

				if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)	//8bitのインデックスバッファであるか判定
				{
					OutputDebugStringA("[GltfModelData Warning] 8-bit index buffer detected. Promoting to 16-bit.\n");

					std::vector<unsigned char> expanded_buffer(gltf_accessor.count * sizeof(uint16_t));	//16bit用に拡張したサイズのバッファを確保
					uint16_t* dest = reinterpret_cast<uint16_t*>(expanded_buffer.data());	//書き込み用の16bitポインタ
					const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;	//読み込み元の8bitポインタ

					for (size_t i = 0; i < gltf_accessor.count; i++)	//全てのインデックスをループ
					{
						dest[i] = src[i];
					}

					raw_buffers.push_back(std::move(expanded_buffer));

					primitive.index_buffer_view.format = DXGI_FORMAT_R16_UINT;
					primitive.index_buffer_view.buffer = static_cast<int>(raw_buffers.size() - 1);
					primitive.index_buffer_view.stride_in_bytes = sizeof(uint16_t);
					primitive.index_buffer_view.byte_offset = 0;
					primitive.index_buffer_view.count = gltf_accessor.count;
				}
				else	//8bit以外の場合
				{
					primitive.index_buffer_view.format = ConvertFormat(gltf_accessor);
					primitive.index_buffer_view.buffer = gltf_buffer_view.buffer;
					primitive.index_buffer_view.stride_in_bytes = gltf_accessor.ByteStride(gltf_buffer_view);
					primitive.index_buffer_view.byte_offset = gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					primitive.index_buffer_view.count = gltf_accessor.count;
				}
			}

			for (std::map<std::string, int>::const_reference gltf_attribute : gltf_primitive.attributes)	//全頂点属性を走査
			{
				const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_attribute.second);	//属性ごとのアクセサを取得
				const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//ビューを取得

				if (gltf_attribute.first == "JOINTS_0" && gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) //ジョイントデータが8ビット整数であるかの条件分岐
				{
					OutputDebugStringA("[GltfModelData Warning] 8-bit bone indices detected. Promoting to 16-bit.\n");

					std::vector<unsigned char> expanded_buffer(gltf_accessor.count * sizeof(uint16_t) * 4); //16ビット整数4要素に拡張するための作業バッファ配列
					uint16_t* dest = reinterpret_cast<uint16_t*>(expanded_buffer.data()); //拡張バッファの書き込み先先頭ポインタ
					const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset; //生のバイト配列から属性データの開始位置を示す読み込み元ポインタ
					size_t src_stride = gltf_accessor.ByteStride(gltf_buffer_view); //元のデータのストライド幅を表すサイズ

					for (size_t i = 0; i < gltf_accessor.count; i++) //すべての頂点要素を走査して型拡張を行うループ
					{
						const uint8_t* src_vertex = src + i * src_stride; //現在の頂点のデータ位置を示すポインタ
						dest[i * 4 + 0] = src_vertex[0];
						dest[i * 4 + 1] = src_vertex[1];
						dest[i * 4 + 2] = src_vertex[2];
						dest[i * 4 + 3] = src_vertex[3];
					}

					raw_buffers.push_back(std::move(expanded_buffer));

					buffer_view vertex_buffer_view = {}; //頂点属性バッファビュー
					vertex_buffer_view.format = DXGI_FORMAT_R16G16B16A16_UINT;
					vertex_buffer_view.buffer = static_cast<int>(raw_buffers.size() - 1);
					vertex_buffer_view.stride_in_bytes = sizeof(uint16_t) * 4;
					vertex_buffer_view.byte_offset = 0;
					vertex_buffer_view.count = gltf_accessor.count;

					primitive.vertex_buffer_views.emplace(std::make_pair(gltf_attribute.first, vertex_buffer_view));
				}
				else if (gltf_attribute.first == "WEIGHTS_0" && gltf_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) //ウェイトデータが浮動小数点数以外で格納されているかの条件分岐
				{
					OutputDebugStringA("[GltfModelData Warning] Non-float bone weights detected. Promoting to 32-bit float.\n");

					std::vector<unsigned char> expanded_buffer(gltf_accessor.count * sizeof(float) * 4); //32ビット浮動小数点数4要素に復元するための作業バッファ配列
					float* dest = reinterpret_cast<float*>(expanded_buffer.data()); //拡張バッファの書き込み先浮動小数点数ポインタ
					const unsigned char* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset; //読み込み元のバイトデータポインタ
					size_t src_stride = gltf_accessor.ByteStride(gltf_buffer_view); //ストライド幅サイズ

					for (size_t i = 0; i < gltf_accessor.count; i++) //すべての頂点要素を走査して正規化デコードを行うループ
					{
						const unsigned char* src_vertex = src + i * src_stride; //現在の頂点ウェイトデータ位置ポインタ
						if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) //元のデータが8ビット整数であるかの条件分岐
						{
							const uint8_t* src_w = reinterpret_cast<const uint8_t*>(src_vertex); //8ビット整数用ポインタ
							dest[i * 4 + 0] = gltf_accessor.normalized ? (src_w[0] / 255.0f) : static_cast<float>(src_w[0]);
							dest[i * 4 + 1] = gltf_accessor.normalized ? (src_w[1] / 255.0f) : static_cast<float>(src_w[1]);
							dest[i * 4 + 2] = gltf_accessor.normalized ? (src_w[2] / 255.0f) : static_cast<float>(src_w[2]);
							dest[i * 4 + 3] = gltf_accessor.normalized ? (src_w[3] / 255.0f) : static_cast<float>(src_w[3]);
						}
						else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) //元のデータが16ビット整数であるかの条件分岐
						{
							const uint16_t* src_w = reinterpret_cast<const uint16_t*>(src_vertex); //16ビット整数用ポインタ
							dest[i * 4 + 0] = gltf_accessor.normalized ? (src_w[0] / 65535.0f) : static_cast<float>(src_w[0]);
							dest[i * 4 + 1] = gltf_accessor.normalized ? (src_w[1] / 65535.0f) : static_cast<float>(src_w[1]);
							dest[i * 4 + 2] = gltf_accessor.normalized ? (src_w[2] / 65535.0f) : static_cast<float>(src_w[2]);
							dest[i * 4 + 3] = gltf_accessor.normalized ? (src_w[3] / 65535.0f) : static_cast<float>(src_w[3]);
						}
					}

					raw_buffers.push_back(std::move(expanded_buffer));

					buffer_view vertex_buffer_view = {}; //頂点属性バッファビュー
					vertex_buffer_view.format = DXGI_FORMAT_R32G32B32_FLOAT;
					vertex_buffer_view.buffer = static_cast<int>(raw_buffers.size() - 1);
					vertex_buffer_view.stride_in_bytes = sizeof(float) * 4;
					vertex_buffer_view.byte_offset = 0;
					vertex_buffer_view.count = gltf_accessor.count;

					primitive.vertex_buffer_views.emplace(std::make_pair(gltf_attribute.first, vertex_buffer_view));
				}
				else //通常の属性データである場合の条件分岐
				{
					buffer_view vertex_buffer_view = {};	//頂点バッファビュー情報を構築
					vertex_buffer_view.format = ConvertFormat(gltf_accessor);
					vertex_buffer_view.buffer = gltf_buffer_view.buffer;
					vertex_buffer_view.stride_in_bytes = gltf_accessor.ByteStride(gltf_buffer_view);
					vertex_buffer_view.byte_offset = gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					vertex_buffer_view.count = gltf_accessor.count;

					primitive.vertex_buffer_views.emplace(std::make_pair(gltf_attribute.first, vertex_buffer_view));
				}
			}
			ComputeTangentsForPrimitive(primitive);
		}
		ConvertMeshAxisSystem(mesh);
	}
}

//=========================================
//tinygltfのモデルからノード情報を抽出
//=========================================
void GltfModelData::FetchNodes(const tinygltf::Model& gltf_model)
{
	for (std::vector<tinygltf::Node>::const_reference gltf_node : gltf_model.nodes)
	{
		node& node = nodes.emplace_back();
		node.name = gltf_node.name;
		node.skin = gltf_node.skin;
		node.mesh = gltf_node.mesh;
		node.children = gltf_node.children;

		if (!gltf_node.matrix.empty())
		{
			DirectX::XMFLOAT4X4 matrix;
			constexpr size_t MATRIX_ELEMENTS = 16;

			for (size_t i = 0; i < MATRIX_ELEMENTS; i++)
			{
				matrix.m[i / MATRIX_DIMENSION][i % MATRIX_DIMENSION] = static_cast<float>(gltf_node.matrix.at(i));
			}

			DirectX::XMMATRIX m = DirectX::XMLoadFloat4x4(&matrix);
			DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(m);
			DirectX::XMVECTOR S, R, T;
			bool succeed = false;

			if (DirectX::XMVector3Less(det, DirectX::XMVectorZero()))
			{
				DirectX::XMMATRIX fixed_m = m;
				fixed_m.r[0] = DirectX::XMVectorNegate(fixed_m.r[0]);
				succeed = DirectX::XMMatrixDecompose(&S, &R, &T, fixed_m);

				if (succeed)
				{
					DirectX::XMFLOAT3 scale_val;
					DirectX::XMStoreFloat3(&scale_val, S);
					scale_val.x = -scale_val.x;
					S = DirectX::XMLoadFloat3(&scale_val);
				}
			}
			else
			{
				succeed = DirectX::XMMatrixDecompose(&S, &R, &T, m);
			}

			if (!succeed)
			{
				OutputDebugStringA("[GltfModelData Warning] FetchNodes: Failed to decompose matrix. Setting default identity.\n");
				S = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
				R = DirectX::XMQuaternionIdentity();
				T = DirectX::XMVectorZero();
			}

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.translation, T);
		}
		else
		{
			if (gltf_node.scale.size() > 0)
			{
				node.scale.x = static_cast<float>(gltf_node.scale.at(0));
				node.scale.y = static_cast<float>(gltf_node.scale.at(1));
				node.scale.z = static_cast<float>(gltf_node.scale.at(2));
			}
			if (gltf_node.translation.size() > 0)
			{
				node.translation.x = static_cast<float>(gltf_node.translation.at(0));
				node.translation.y = static_cast<float>(gltf_node.translation.at(1));
				node.translation.z = static_cast<float>(gltf_node.translation.at(2));
			}
			if (gltf_node.rotation.size() > 0)
			{
				node.rotation.x = static_cast<float>(gltf_node.rotation.at(0));
				node.rotation.y = static_cast<float>(gltf_node.rotation.at(1));
				node.rotation.z = static_cast<float>(gltf_node.rotation.at(2));
				node.rotation.w = static_cast<float>(gltf_node.rotation.at(3));
			}
		}

		ConvertNodeAxisSystem(node);
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

	for (vector<Skin>::const_reference transmission_skin : gltf_model.skins)	//モデル内のすべてのスキン情報をループ
	{
		skin& skin = skins.emplace_back();	//新しいスキン要素を追加
		const Accessor& gltf_accessor = gltf_model.accessors.at(transmission_skin.inverseBindMatrices);	//逆バインド行列のアクセサを取得
		const BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//対応するバッファビューを取得
		skin.inverse_bind_matrices.resize(gltf_accessor.count);
		memcpy(
			skin.inverse_bind_matrices.data(),
			gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
			gltf_accessor.count * sizeof(XMFLOAT4X4));
		skin.joints = transmission_skin.joints;

		for (size_t i = 0; i < skin.inverse_bind_matrices.size(); i++)	//読み込んだ全ての逆バインド行列をループ
		{
			DirectX::XMFLOAT4X4& m = skin.inverse_bind_matrices.at(i); //対象の逆バインド行列

			m._12 = -m._12;
			m._13 = -m._13;
			m._21 = -m._21;
			m._31 = -m._31;
			m._41 = -m._41;
		}

		skin.joints = transmission_skin.joints;
	}

	for (vector<Animation>::const_reference gltf_animation : gltf_model.animations)	//全アニメーションデータをループ
	{
		animation& animation = animations.emplace_back();	//アニメーション構造体を追加
		animation.name = gltf_animation.name;
		for (vector<AnimationSampler>::const_reference gltf_sampler : gltf_animation.samplers)	//アニメーション内の全サンプラーをループ
		{
			animation::sampler& sampler = animation.samplers.emplace_back();	//サンプラー構造体を追加
			sampler.input = gltf_sampler.input;
			sampler.output = gltf_sampler.output;
			sampler.interpolation = gltf_sampler.interpolation;

			const Accessor& gltf_accessor = gltf_model.accessors.at(gltf_sampler.input);	//入力アクセサを取得
			const BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//入力ビューを取得
			const pair<unordered_map<int, vector<float>>::iterator, bool>& timelines{ animation.timelines.emplace(gltf_sampler.input, gltf_accessor.count) };	//タイムライン配列を確保
			if (timelines.second)	//新規に確保が成功したかの条件分岐
			{
				memcpy(
					timelines.first->second.data(),
					gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
					gltf_accessor.count * sizeof(FLOAT));
			}
		}
		for (vector<AnimationChannel>::const_reference gltf_channel : gltf_animation.channels)	//全チャンネルをループ
		{
			animation::channel& channel = animation.channels.emplace_back();	//チャンネル構造体を追加
			channel.sampler = gltf_channel.sampler;
			channel.target_node = gltf_channel.target_node;
			channel.target_path = gltf_channel.target_path;

			const AnimationSampler& gltf_sampler = gltf_animation.samplers.at(gltf_channel.sampler);	//対応サンプラーを取得
			const Accessor& gltf_accessor = gltf_model.accessors.at(gltf_sampler.output);	//出力アクセサを取得
			const BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);	//出力ビューを取得

			if (gltf_channel.target_path == "scale")	//スケールに対するアニメーションチャネルであるかの条件分岐
			{
				const pair<unordered_map<int, vector<XMFLOAT3>>::iterator, bool>& scales = animation.scales.emplace(gltf_sampler.output, gltf_accessor.count);
				if (scales.second)	//新規に確保が成功したかの条件分岐
				{
					memcpy(
						scales.first->second.data(),
						gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
						gltf_accessor.count * sizeof(XMFLOAT3));
				}
			}
			else if (gltf_channel.target_path == "rotation")	//回転に対するアニメーションチャネルであるかの条件分岐
			{
				const pair<unordered_map<int, vector<XMFLOAT4>>::iterator, bool>& rotations = animation.rotations.emplace(gltf_sampler.output, gltf_accessor.count);
				if (rotations.second)	//新規に確保が成功したかの条件分岐
				{
					memcpy(
						rotations.first->second.data(),
						gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
						gltf_accessor.count * sizeof(XMFLOAT4));
				}
			}
			else if (gltf_channel.target_path == "translation")	//位置に対するアニメーションチャネルであるかの条件分岐
			{
				const pair<unordered_map<int, vector<XMFLOAT3>>::iterator, bool>& translations = animation.translations.emplace(gltf_sampler.output, gltf_accessor.count);

				if (translations.second)	//新規に確保が成功したかの条件分岐
				{
					memcpy(
						translations.first->second.data(),
						gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset,
						gltf_accessor.count * sizeof(XMFLOAT3));
				}
			}
		}
	}
	for (decltype(animations)::reference animation : animations) //全アニメーションの再生時間を確定するループ
	{
		for (decltype(animation.timelines)::reference timelines : animation.timelines) //全タイムラインを走査するループ
		{
			animation.duration = std::max<float>(animation.duration, timelines.second.back());
		}
		ConvertAnimationAxisSystem(animation);
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

//モデルデータに接線（TANGENT）情報が含まれていない場合、頂点座標とUV座標から接線ベクトルを自動計算
void GltfModelData::ComputeTangentsForPrimitive(mesh::primitive& primitive)
{
	if (primitive.has("TANGENT") || !primitive.has("POSITION") || !primitive.has("TEXCOORD_0") || primitive.index_buffer_view.buffer == -1)
	{
		return;
	}

	const buffer_view& pos_view = primitive.vertex_buffer_views.at("POSITION");
	const buffer_view& uv_view = primitive.vertex_buffer_views.at("TEXCOORD_0");
	const buffer_view& idx_view = primitive.index_buffer_view;
	const buffer_view& norm_view = primitive.has("NORMAL") ? primitive.vertex_buffer_views.at("NORMAL") : pos_view;

	size_t vertex_count = pos_view.count;
	std::vector<DirectX::XMFLOAT3> tan1(vertex_count, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
	std::vector<DirectX::XMFLOAT3> tan2(vertex_count, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

	const unsigned char* pos_data = raw_buffers.at(pos_view.buffer).data() + pos_view.byte_offset;
	const unsigned char* uv_data = raw_buffers.at(uv_view.buffer).data() + uv_view.byte_offset;
	const unsigned char* idx_data = raw_buffers.at(idx_view.buffer).data() + idx_view.byte_offset;

	for (size_t i = 0; i < idx_view.count; i += 3)
	{
		uint32_t i1 = 0, i2 = 0, i3 = 0;

		if (idx_view.format == DXGI_FORMAT_R16_UINT)
		{
			const uint16_t* idx = reinterpret_cast<const uint16_t*>(idx_data + i * idx_view.stride_in_bytes);
			i1 = idx[0]; i2 = idx[1]; i3 = idx[2];
		}
		else if (idx_view.format == DXGI_FORMAT_R32_UINT)
		{
			const uint32_t* idx = reinterpret_cast<const uint32_t*>(idx_data + i * idx_view.stride_in_bytes);
			i1 = idx[0]; i2 = idx[1]; i3 = idx[2];
		}

		const DirectX::XMFLOAT3& v1 = *reinterpret_cast<const DirectX::XMFLOAT3*>(pos_data + i1 * pos_view.stride_in_bytes);
		const DirectX::XMFLOAT3& v2 = *reinterpret_cast<const DirectX::XMFLOAT3*>(pos_data + i2 * pos_view.stride_in_bytes);
		const DirectX::XMFLOAT3& v3 = *reinterpret_cast<const DirectX::XMFLOAT3*>(pos_data + i3 * pos_view.stride_in_bytes);

		const DirectX::XMFLOAT2& w1 = *reinterpret_cast<const DirectX::XMFLOAT2*>(uv_data + i1 * uv_view.stride_in_bytes);
		const DirectX::XMFLOAT2& w2 = *reinterpret_cast<const DirectX::XMFLOAT2*>(uv_data + i2 * uv_view.stride_in_bytes);
		const DirectX::XMFLOAT2& w3 = *reinterpret_cast<const DirectX::XMFLOAT2*>(uv_data + i3 * uv_view.stride_in_bytes);

		float x1 = v2.x - v1.x; float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y; float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z; float z2 = v3.z - v1.z;
		float s1 = w2.x - w1.x; float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y; float t2 = w3.y - w1.y;

		float r = 1.0f / (s1 * t2 - s2 * t1);
		DirectX::XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		DirectX::XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1.at(i1).x += sdir.x; tan1.at(i1).y += sdir.y; tan1.at(i1).z += sdir.z;
		tan1.at(i2).x += sdir.x; tan1.at(i2).y += sdir.y; tan1.at(i2).z += sdir.z;
		tan1.at(i3).x += sdir.x; tan1.at(i3).y += sdir.y; tan1.at(i3).z += sdir.z;

		tan2.at(i1).x += tdir.x; tan2.at(i1).y += tdir.y; tan2.at(i1).z += tdir.z;
		tan2.at(i2).x += tdir.x; tan2.at(i2).y += tdir.y; tan2.at(i2).z += tdir.z;
		tan2.at(i3).x += tdir.x; tan2.at(i3).y += tdir.y; tan2.at(i3).z += tdir.z;
	}

	std::vector<unsigned char> tangent_buffer(vertex_count * sizeof(DirectX::XMFLOAT4));
	unsigned char* norm_base_data = raw_buffers.at(norm_view.buffer).data() + norm_view.byte_offset;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		const DirectX::XMFLOAT3& n_val = *reinterpret_cast<const DirectX::XMFLOAT3*>(norm_base_data + i * norm_view.stride_in_bytes);
		DirectX::XMVECTOR N = DirectX::XMLoadFloat3(&n_val);
		DirectX::XMVECTOR T1 = DirectX::XMLoadFloat3(&tan1.at(i));

		DirectX::XMVECTOR T = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(T1, DirectX::XMVectorScale(N, DirectX::XMVectorGetX(DirectX::XMVector3Dot(N, T1)))));
		DirectX::XMVECTOR T2 = DirectX::XMLoadFloat3(&tan2.at(i));
		float handedness = (DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(N, T1), T2)) < 0.0f) ? -1.0f : 1.0f;

		DirectX::XMFLOAT4 final_tangent;
		DirectX::XMStoreFloat4(&final_tangent, DirectX::XMVectorSetW(T, handedness));
		reinterpret_cast<DirectX::XMFLOAT4*>(tangent_buffer.data())[i] = final_tangent;
	}

	raw_buffers.push_back(std::move(tangent_buffer));

	buffer_view tang_view = {};
	tang_view.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	tang_view.buffer = static_cast<int>(raw_buffers.size() - 1);
	tang_view.stride_in_bytes = sizeof(DirectX::XMFLOAT4);
	tang_view.byte_offset = 0;
	tang_view.count = vertex_count;

	primitive.vertex_buffer_views.emplace("TANGENT", tang_view);
}

//メッシュ内の頂点データ（座標、法線、接線）のX軸を反転してDirectX仕様の左手系に変換
void GltfModelData::ConvertMeshAxisSystem(mesh& target_mesh)
{
	for (mesh::primitive& primitive : target_mesh.primitives)
	{
		if (primitive.has("POSITION"))
		{
			buffer_view& view = primitive.vertex_buffer_views.at("POSITION");
			unsigned char* data = raw_buffers.at(view.buffer).data() + view.byte_offset;

			for (size_t i = 0; i < view.count; ++i)
			{
				DirectX::XMFLOAT3* pos = reinterpret_cast<DirectX::XMFLOAT3*>(data + i * view.stride_in_bytes);
				pos->x = -pos->x;
			}
		}

		if (primitive.has("NORMAL"))
		{
			buffer_view& view = primitive.vertex_buffer_views.at("NORMAL");
			unsigned char* data = raw_buffers.at(view.buffer).data() + view.byte_offset;

			for (size_t i = 0; i < view.count; ++i)
			{
				DirectX::XMFLOAT3* norm = reinterpret_cast<DirectX::XMFLOAT3*>(data + i * view.stride_in_bytes);
				norm->x = -norm->x;
			}
		}

		if (primitive.has("TANGENT"))
		{
			buffer_view& view = primitive.vertex_buffer_views.at("TANGENT");
			unsigned char* data = raw_buffers.at(view.buffer).data() + view.byte_offset;

			for (size_t i = 0; i < view.count; ++i)
			{
				DirectX::XMFLOAT4* tang = reinterpret_cast<DirectX::XMFLOAT4*>(data + i * view.stride_in_bytes);
				tang->x = -tang->x;
			}
		}

		if (primitive.index_buffer_view.buffer > -1)
		{
			buffer_view& view = primitive.index_buffer_view;
			unsigned char* data = raw_buffers.at(view.buffer).data() + view.byte_offset;

			for (size_t i = 0; i < view.count; i += 3)
			{
				if (view.format == DXGI_FORMAT_R16_UINT)
				{
					uint16_t* idx = reinterpret_cast<uint16_t*>(data + i * view.stride_in_bytes);
					uint16_t temp = idx[1];
					idx[1] = idx[2];
					idx[2] = temp;
				}
				else if (view.format == DXGI_FORMAT_R32_UINT)
				{
					uint32_t* idx = reinterpret_cast<uint32_t*>(data + i * view.stride_in_bytes);
					uint32_t temp = idx[1];
					idx[1] = idx[2];
					idx[2] = temp;
				}
			}
		}
	}
}

//各ボーン・ノードの初期ポーズ（TRSプロパティ）の座標系を右手系から左手系に変換
void GltfModelData::ConvertNodeAxisSystem(node& target_node)
{
	target_node.translation.x = -target_node.translation.x;

	target_node.rotation.x = -target_node.rotation.x;
	target_node.rotation.z = -target_node.rotation.z;
}

//アニメーションの全キーフレーム（位置、回転）のデータを左手系に変換
void GltfModelData::ConvertAnimationAxisSystem(animation& target_animation)
{
	for (auto& pair : target_animation.translations) // すべての移動キーフレームマップを走査するループ
	{
		for (DirectX::XMFLOAT3& trans : pair.second)
		{
			trans.x = -trans.x;
		}
	}

	for (auto& pair : target_animation.rotations) // すべての回転キーフレームマップを走査するループ
	{
		for (DirectX::XMFLOAT4& rot : pair.second)
		{
			rot.x = -rot.x;
			rot.w = -rot.w;
		}
	}
}
