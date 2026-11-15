#pragma once

#include <string>
#include <DirectXMath.h>

class GameObject;

//抽出する情報
struct ExtractedObjectData
{
	DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f };		//座標
	DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };	//回転
	DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f };			//拡大縮小
	std::string model_path = "";							//モデルパス
};

class ObjectClassConverter
{
public:
	//クラス変換
	GameObject* ConvertClass(GameObject* old_object, const std::string& target_class_name);

private:
	//旧情報の抽出
	ExtractedObjectData ExtractObjectData(GameObject* old_object)const;

	//新オブジェクトの構築
	void CreateNewObject(GameObject* new_object, const ExtractedObjectData& data)const;
};

