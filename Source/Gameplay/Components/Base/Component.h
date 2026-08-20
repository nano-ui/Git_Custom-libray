#pragma once

#include <string>

class JsonSerializer;
class GuiInspector;
struct ID3D11DeviceContext;

class Component
{
public:
	//コンストラクタ
	Component();

	//仮想デストラクタ
	virtual ~Component();

	//初期化処理
	virtual void Initialize();

	//更新処理
	virtual void Update(float elapsed_time) = 0;

	//描画処理
	virtual void Render(ID3D11DeviceContext* context){}

	//デバッグ描画処理
	virtual void RenderGui();

	//JSON保存用変数登録
	virtual void SetupSerialization(JsonSerializer* serializer){}

	//GuiInspector専用のインスペクター項目登録
	virtual void SetupInspector(GuiInspector* inspector){}

	//有効フラグ取得
	bool IsActive()const { return is_active; }

	//有効フラグの設定
	void SetActive(bool active) { is_active = active; }

	//コンポーネント名の取得
	const std::string& GetComponentName()const { return component_name; }

	//コンポーネント名の設定
	void SetComponentName(const std::string& name) { component_name = name; }

protected:
	bool is_active;				//有効フラグ
	std::string component_name;	//デバッグ用コンポーネント名
};

