#include "Component.h"

#include <Windows.h>

//コンストラクタ
Component::Component()
	:is_active(true)
	, component_name("Component")
{
}

//仮想デストラクタ
Component::~Component()
{
}

//初期化処理
void Component::Initialize()
{
	if(component_name.empty()) OutputDebugStringA("[Component 警告] Component::Initialize: コンポーネント名が空のまま初期化されました。\n");
}

//デバッグ描画処理
void Component::RenderGui()
{
	if (!is_active)return;
}

//JSON保存用変数登録
void Component::SetupSerialization(JsonSerializer* serializer)
{
	if (!serializer)
	{
		OutputDebugStringA("[Component::SetupSerialization] serializerがnullです。\n");
		return;
	}
}

//GuiInspector専用のインスペクター項目登録
void Component::SetupInspector(GuiInspector* inspector)
{
	if (!inspector)
	{
		OutputDebugStringA("[Component::SetupInspector] inspectorがnullです。\n");
		return;
	}
}
