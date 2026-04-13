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

	//------------------------------------------------------------------
	//ローカル行列の計算準備
	//------------------------------------------------------------------

	DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity();

	//------------------------------------------------------------------
	//データの保持形式に応じた解析
	//------------------------------------------------------------------

	//16個の数値を持つ行列データがある場合
	if (node.matrix.size() == 16)
	{
		transform = DirectX::XMMatrixSet(
			static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]), static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
			static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]), static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
			static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]), static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
			static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]), static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15])
		);
	}
	//行列ではなく、移動・回転・拡大縮小の個別データがある場合
	else
	{
		//拡大縮小の適用
		if (node.scale.size() == 3)
		{
			transform *= DirectX::XMMatrixScaling(
				static_cast<float>(node.scale[0]),
				static_cast<float>(node.scale[1]),
				static_cast<float>(node.scale[2])
			);
		}

		//回転の適用
		if (node.rotation.size() == 4)
		{
			if (node.rotation.size() == 4)
			{
				transform *= DirectX::XMMatrixRotationQuaternion(
					DirectX::XMVectorSet(
						static_cast<float>(node.rotation[0]),
						static_cast<float>(node.rotation[1]),
						static_cast<float>(node.rotation[2]),
						static_cast<float>(node.rotation[3])));
			}
		}

		//移動の適用
		if (node.translation.size() == 3)
		{
			transform *= DirectX::XMMatrixTranslation(
				static_cast<float>(node.translation[0]),
				static_cast<float>(node.translation[1]),
				static_cast<float>(node.translation[2])
			);
		}
	}

	//------------------------------------------------------------------
	//ローカル行列の保存
	//------------------------------------------------------------------

	DirectX::XMStoreFloat4x4(&local_matrix, transform);
}