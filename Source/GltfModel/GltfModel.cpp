#include "GltfModel.h"
#define TINYGLTF_IMPLEMENTATION
#include "tinygltf-release/tiny_gltf.h"
#include "misc.h"
#include "../Graphics/shader.h"

//==============================================
//画像読み込みをスキップするためのダミー関数
//==============================================
bool null_load_image_data(tinygltf::Image*, const int, std::string*, std::string*,
	int, int, const unsigned char*, int, void*)
{
	//常に成功を返した画像処理をバイパスする
	return true;
}

//==================================================
//デバイスとファイルを受け取ってモデルを初期化
//==================================================
GltfModel::GltfModel(ID3D11Device* device, const std::string& filename)
{
	//------------------------------
	//シェーダーオブジェクトの生成
	//------------------------------
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT		,0,0,D3D11_INPUT_PER_VERTEX_DATA, 0},	//頂点座標の定義
		{ "NORMAL"	,0,DXGI_FORMAT_R32G32B32_FLOAT		,1,0,D3D11_INPUT_PER_VERTEX_DATA,0 },	//法線ベクトルの定義
		{ "TANGENT"	,0,DXGI_FORMAT_R32G32B32A32_FLOAT	,2,0,D3D11_INPUT_PER_VERTEX_DATA,0 },	//接線ベクトルの定義
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT			,3,0,D3D11_INPUT_PER_VERTEX_DATA,0 },	//テクスチャ座標の定義
		{ "JOINTS"	,0,DXGI_FORMAT_R16G16B16A16_UINT	,4,0,D3D11_INPUT_PER_VERTEX_DATA,0 },	//ボーンインデックスの定義
		{ "WEIGHTS"	,0,DXGI_FORMAT_R32G32B32A32_FLOAT	,5,0,D3D11_INPUT_PER_VERTEX_DATA,0 }	//スキンウェイトの定義
	};
	
	create_vs_from_cso(		//csoファイルから頂点シェーダーと入力レイアウトを作成
		device, 
		"gltf_model_vs.cso",
		vertex_shader.ReleaseAndGetAddressOf(),
		input_layout.ReleaseAndGetAddressOf(),
		input_element_desc,
		_countof(input_element_desc)
	);

	create_ps_from_cso(		//csoファイルからピクセルシェーダーを作成
		device,
		"gltf_model_ps.cso",
		pixel_shader.ReleaseAndGetAddressOf()
	);

	D3D11_BUFFER_DESC buffer_desc = {};	//プリミティブ定数バッファの設定
	buffer_desc.ByteWidth = sizeof(primitive_constants);	//構造体のサイズを設定
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;				//デフォルトの使用法を設定
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;		//定数バッファとしてバインド
	HRESULT hr;
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_cbuffer.ReleaseAndGetAddressOf());	//定数バッファの作成
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));	//作成できたかチェック

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
	FetchMeshes(device, gltf_model);	//メッシュデータの抽出とバッファの生成
	FetchNodes(gltf_model);				//ノード情報の抽出と階層行列の計算
}

//=============
//モデル描画
//=============
void GltfModel::Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world)
{
	using namespace DirectX;

	//----------------------------------------
	//シェーダーとパイプラインステートの設定
	//----------------------------------------
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);	//頂点シェーダーをバインド
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);		//ピクセルシェーダーをバインド
	immediate_context->IASetInputLayout(input_layout.Get());			//入力レイアウトをバインド
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	//トポロジーを三角形リストに設定

	//-----------------------------------
	//ノード階層を再帰的に走査して描画
	//-----------------------------------
	std::function<void(int)> traverse{ [&](int node_index)->void {
		const node& node = nodes.at(node_index);	//ノード情報を取得
		if (node.mesh > -1)	//メッシュが存在するかチェック
		{
			const mesh& mesh = meshes.at(node.mesh);	//描画対象のメッシュを取得
			for (std::vector<mesh::primitive>::const_reference primitive : mesh.primitives)	//メッシュ内の全プリミティブをループ
			{
				//属性ごとにバッファを取り出し配列に格納
				ID3D11Buffer* vertex_buffers[]
				{
					primitive.has("POSITION") ?
					buffers.at(primitive.vertex_buffer_views.at("POSITION").buffer).Get() : NULL,
					primitive.has("NORMAL") ?
					buffers.at(primitive.vertex_buffer_views.at("NORMAL").buffer).Get() : NULL,
					primitive.has("TANGENT") ?
					buffers.at(primitive.vertex_buffer_views.at("TANGENT").buffer).Get() : NULL,
					primitive.has("TEXCOORD_0") ?
					buffers.at(primitive.vertex_buffer_views.at("TEXCOORD_O").buffer).Get() : NULL,
					primitive.has("JOINTS_0") ?
					buffers.at(primitive.vertex_buffer_views.at("JOINTS_0").buffer).Get() : NULL,
					primitive.has("WEIGHTS_0") ?
					buffers.at(primitive.vertex_buffer_views.at("WEIGHTS_0").buffer).Get() : NULL,
				};
				//各バッファのストライド（1要素のサイズ）を取得し配列に格納
				UINT strides[]
				{
					primitive.has("POSITION") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").stride_in_bytes) : 0,
					primitive.has("NORMAL") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").stride_in_bytes) : 0,
					primitive.has("TANGENT") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").stride_in_bytes) : 0,
					primitive.has("TEXCOORD_0") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").stride_in_bytes) : 0,
					primitive.has("JOINTS_0") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").stride_in_bytes) : 0,
					primitive.has("WEIGHTS_0") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").stride_in_bytes) : 0,
				};
				//各バッファの開始位置を取得し配列に格納
				UINT offsets[]
				{
					primitive.has("POSITION") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").byte_offset) : 0,
					primitive.has("NORMAL") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").byte_offset) : 0,
					primitive.has("TANGENT") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").byte_offset) : 0,
					primitive.has("TEXCOORD_0") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").byte_offset) : 0,
					primitive.has("JOINTS_0") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").byte_offset) : 0,
					primitive.has("WEIGHTS_0") ?
					static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").byte_offset) : 0,
				};
				immediate_context->IASetVertexBuffers(0, _countof(vertex_buffers), vertex_buffers, strides, offsets);	//頂点バッファをスロットにセット


				primitive_constants primitive_data = {};	//プリミティブ単位の定数データを設定
				primitive_data.material = primitive.material;	//マテリアルIDを設定
				primitive_data.has_tangent = primitive.has("TANGENT");	//接線の有無を設定
				primitive_data.skin = node.skin;	//スキン情報を設定
				XMStoreFloat4x4(&primitive_data.world, XMLoadFloat4x4(&node.global_transform) * XMLoadFloat4x4(&world));	//ローカルのグローバル変換に行列を掛け合わせてワールド行列を算出
				immediate_context->UpdateSubresource(primitive_cbuffer.Get(), 0, 0, &primitive_data, 0, 0);	//定数バッファの中身をGPUに更新
				immediate_context->VSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());	//頂点シェーダーにプリミティブ定数バッファを設定
				immediate_context->PSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());	//ピクセルシェーダーにプリミティブ定数バッファを設定

				//--------------
				//描画命令
				///-------------
				if (primitive.index_buffer_view.buffer > -1)	//インデックスバッファの有無をチェック
				{
					//インデックスバッファをバインド
					immediate_context->IASetIndexBuffer(buffers.at(primitive.index_buffer_view.buffer).Get(),
						primitive.index_buffer_view.format, static_cast<UINT>(primitive.index_buffer_view.byte_offset));
					immediate_context->DrawIndexed(static_cast<UINT>(primitive.index_buffer_view.count), 0, 0);	//素引き付き描画を実行
				}
				else
				{
					immediate_context->Draw(static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").count), 0);	//頂点数に元図いて直接描画を実行
				}
			}
		}
		//子ノードに対しても同じ処理を再帰的に実行
		for (std::vector<int>::value_type child_index : node.children)
		{
			traverse(child_index);
		}
	} };

	//------------------------------------
	//ルートノードからトラバースを開始
	//------------------------------------
	for (std::vector<int>::value_type node_index : scenes.at(default_scene).nodes)
	{
		traverse(node_index);
	}
}

//=========================================
//tinygltfのモデルからノード情報を抽出
//=========================================
void GltfModel::FetchNodes(const tinygltf::Model& gltf_model)
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
			for (size_t row = 0; row < 4; row++)	//行ループを行う
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
	CumulateTransforms(nodes);	//全ノードの抽出完了後、階層行列を再帰的に計算
}

//======================================================
//親子関係をたどって各ノードのグローバル行列を計算
//======================================================
void GltfModel::CumulateTransforms(std::vector<node>& nodes)
{
	using namespace DirectX;	//DirectXMathの名前空間を使用

	std::stack<XMFLOAT4X4> parent_global_transforms;	//親ノードの行列を保持するスタック
	std::function<void(int)> traverse{ [&](int node_index)->void	//階層を再帰的に巡回
	{
		node& node = nodes.at(node_index);	//対象のノードを参照
		XMMATRIX S = XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);	//スケール行列を作成
		XMMATRIX R = XMMatrixRotationQuaternion(XMVectorSet(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w));	//回転行列を作成
		XMMATRIX T = XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);	//移動行列を作成
		XMStoreFloat4x4(&node.global_transform, S * R * T * XMLoadFloat4x4(&parent_global_transforms.top()));	//親の行列を掛けてグローバル行れを設定
		
		//----------------------
		//子ノードへの再起処理
		//----------------------
		for (int child_index : node.children)	//全ての子ノードに対してループ
		{
			parent_global_transforms.push({ node.global_transform });	//現在の行列を「親の行列」としてスタックに設定
			traverse(child_index);	//子ノードに対して再帰呼び出し
			parent_global_transforms.pop();	//処理が終わったらスタックから取り除く
		}
	} };

	//-----------------------------
	//ルートノードからの巡回処理
	//-----------------------------
	for (std::vector<int>::value_type node_index : scenes.at(default_scene).nodes)	//デフォルトのルートノードをループ
	{
		parent_global_transforms.push({		//単位行列をストックの底に設定
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
			});
		traverse(node_index);	//ルートから探索を開始
		parent_global_transforms.pop();	//終了後にスタックをクリア
	}
}

//=================================================
//tinygltfの型情報をDirectXのフォーマットに変換
//=================================================
DXGI_FORMAT GltfModel::ConvertFormat(const tinygltf::Accessor& accessor)
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
void GltfModel::FetchMeshes(ID3D11Device* device, const tinygltf::Model& gltf_model)
{
	HRESULT hr;	//APIの実行結果を格納

	//-------------------------
	//GPUバッファの一括生成
	//-------------------------
	size_t gltf_buffer_count = gltf_model.buffers.size();		//gltfに含まれるバッファ数取得
	buffers.resize(gltf_buffer_count);	//格納用ベクトルをリサイズ

	for (size_t gltf_buffer_index = 0; gltf_buffer_index < gltf_buffer_count; gltf_buffer_index++)	//全バッファを処理
	{
		const tinygltf::Buffer& gltf_buffer = gltf_model.buffers.at(gltf_buffer_index);	//ソースバッファを参照

		D3D11_BUFFER_DESC buffer_desc = {};	//DirectXバッファ設定用構造体
		buffer_desc.ByteWidth = static_cast<UINT>(gltf_buffer.data.size());	//バッファ全体のサイズ指定
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;	//GPUによる読み書きが可能に設定
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;	//頂点とインデックス両方に使用可能
		
		D3D11_SUBRESOURCE_DATA subresource_data = {};	//初期データ転送用構造体
		subresource_data.pSysMem = gltf_buffer.data.data();	//CPU側のデータポインタを渡す

		hr = device->CreateBuffer(&buffer_desc, &subresource_data, buffers.at(gltf_buffer_index).GetAddressOf());	//バッファを作成
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));	//作成成功かチェック
	}

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
