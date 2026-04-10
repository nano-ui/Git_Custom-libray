#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>

class GltfBone
{
public:
	GltfBone();

	//ボーン名の設定
	void SetBoneName(const std::string& name) { bone_name = name; }
	//ボーン名の取得
	const std::string& GetBoneName() const { return bone_name; }

	//親ボーンのインデックス設定
	void SetParentIndex(int index) { parent_index = index; }
	//親ボーンのインデックス取得
	int GetParentIndex() const { return parent_index; }

	//ローカル行列の設定
	void SetLocalMatrix(const DirectX::XMFLOAT4X4& matrix) { local_matrix = matrix; }
	//ローカル行列の取得
	const DirectX::XMFLOAT4X4& GetLocalMatrix() const { return local_matrix; }

	//グローバル行列の設定
	void SetGlobalMatrix(const DirectX::XMFLOAT4X4& matrix) { global_matrix = matrix; }
	//グローバル行列の取得
	const DirectX::XMFLOAT4X4& GetGlobalMatrix() const { return global_matrix; }

	//オフセット行列の設定
	void SetOffsetMatrix(const DirectX::XMFLOAT4X4& matrix) { offset_matrix = matrix; }
	//オフセット行列の取得
	const DirectX::XMFLOAT4X4& GetOffsetMatrix() const { return offset_matrix; }

private:
	std::string bone_name;					//ボーンの名前
	int parent_index;						//親ボーンのインデックス（-1ならルート）

	DirectX::XMFLOAT4X4 local_matrix;				//親ボーンからの相対的な位置・回転・スケール
	DirectX::XMFLOAT4X4 global_matrix;				//モデルの原点からの最終的な位置・回転・スケール
	DirectX::XMFLOAT4X4 offset_matrix;				//メッシュをボーンの座標系へ変換するための逆行列
};