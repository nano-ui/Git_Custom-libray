#pragma once

#include <string>
#include "../Serialization/json.hpp"

class GameObject;

class EditorConponents
{
public:
	//コンストラクタ
	EditorConponents(GameObject* owner_object)
		:owner(owner_object)
		, is_active(true)
	{

	}

	//デストラクタ
	virtual ~EditorConponents() = default;

	//初期化
	virtual void Initialize() {};

	//更新
	virtual void Update(float elapsed_time) = 0;

	//シリアライズの窓口
	virtual void SetupSerialization(class JsonSerializer* serializer){}

	//シリアライズ読み込み完了後のフック
	virtual void OnPostLoad(){}

	//ゲームオブジェクトを取得
	GameObject* GetGameObjct()const { return owner; }

	//有効フラグ取得
	bool IsActive()const { return is_active; }

	//有効フラグ設定
	void SetActive(bool active_flag) { is_active = active_flag; }

protected:
	GameObject* owner;	//コンポーネントを保持しているGameObject
	bool is_active;		//コンポーネントの有効フラグ
};

