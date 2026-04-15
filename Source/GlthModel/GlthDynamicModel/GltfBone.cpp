#include "GltfBone.h"
#include <tiny_gltf.h>

//=================================================================
//コンストラクタ
//=================================================================	
GltfBone::GltfBone()
{
	parent_index = -1;	//親ボーンなし

	//-----------------------------------------------------------------
	//行列の初期化
	//-----------------------------------------------------------------
	DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();	//DirectXMathを使用して単位行列を作成

	DirectX::XMStoreFloat4x4(&local_matrix, identity);	//ローカル行列を単位行列で初期化
	DirectX::XMStoreFloat4x4(&global_matrix, identity);	//グローバル行列を単位行列で初期化
	DirectX::XMStoreFloat4x4(&offset_matrix, identity);	//オフセット行列を単位行列で初期化
}

//=================================================================
//ボーン情報を初期化
//=================================================================
void GltfBone::Initalize(const tinygltf::Node& node, int parent_idx)
{
	//------------------------------------------------------------------
	//基本情報の設定
	//------------------------------------------------------------------

	bone_name = node.name;				//ボーン名を設定
	parent_index = parent_idx;			//親ボーンのインデックスを設定

	//------------------
	//分解用変数の用意
	//------------------

	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT4 rot = { 0.0f,0.0f,0.0f,1.0f };
	DirectX::XMFLOAT3 scl = { 1.0f,1.0f,1.0f };


	//------------------------------------------------------------------
	//データの保持形式に応じた解析
	//------------------------------------------------------------------

	//16個の数値を持つ行列データがある場合
	if (node.matrix.size() == 16)
	{
		DirectX::XMFLOAT4X4 m(
			static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[12]),
			static_cast<float>(node.matrix[1]), static_cast<float>(node.matrix[5]), static_cast<float>(node.matrix[9]), static_cast<float>(node.matrix[13]),
			static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[14]),
			static_cast<float>(node.matrix[3]), static_cast<float>(node.matrix[7]), static_cast<float>(node.matrix[11]), static_cast<float>(node.matrix[15]));
	
		DirectX::XMVECTOR v_s, v_r, v_t;
		DirectX::XMMatrixDecompose(&v_s, &v_r, &v_t, DirectX::XMLoadFloat4x4(&m));
		DirectX::XMStoreFloat3(&scl, v_s);
		DirectX::XMStoreFloat4(&rot, v_r);
		DirectX::XMStoreFloat3(&pos, v_t);
	}
	//行列ではなく、移動・回転・拡大縮小の個別データがある場合
	else
	{
		//拡大縮小の適用
		if (node.scale.size() == 3)
		{
			scl = {
				static_cast<float>(node.scale[0]),
				static_cast<float>(node.scale[1]),
				static_cast<float>(node.scale[2]) 
			};
		}

		//回転の適用
		if (node.rotation.size() == 4)
		{
			if (node.rotation.size() == 4)
			{
				rot = {
					static_cast<float>(node.rotation[0]),
					static_cast<float>(node.rotation[1]),
					static_cast<float>(node.rotation[2]),
					static_cast<float>(node.rotation[3])
				};
			}
		}

		//移動の適用
		if (node.translation.size() == 3)
		{
			pos = {
				static_cast<float>(node.translation[0]),
				static_cast<float>(node.translation[1]),
				static_cast<float>(node.translation[2])
			};
		}
	}

	//-----------
	//座標変換
	//-----------

	pos.x = -pos.x;
	rot.x = -rot.x;
	rot.w = -rot.w;

	//------------------------------------------------------------------
	//ローカル行列の構築と保存
	//------------------------------------------------------------------

	DirectX::XMMATRIX local =
		DirectX::XMMatrixScaling(scl.x, scl.y, scl.z) *
		DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rot)) *
		DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

	DirectX::XMStoreFloat4x4(&local_matrix, local);

	initial_local_matrix = local_matrix;
}