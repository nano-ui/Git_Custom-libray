#include "Component.h"

#include <Windows.h>
#include <Engine\Core\MacroTracy.h>

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
	BS_PROFILE_FUNCTION();

	if(component_name.empty()) OutputDebugStringA("[Component 警告] Component::Initialize: コンポーネント名が空のまま初期化されました。\n");
}

//デバッグ描画処理
void Component::RenderGui()
{
	BS_PROFILE_FUNCTION();

	if (!is_active)return;
}

//JSON保存用変数登録
void Component::SetupSerialization(JsonSerializer* serializer)
{
	BS_PROFILE_FUNCTION();

	if (!serializer)
	{
		OutputDebugStringA("[Component::SetupSerialization] serializerがnullです。\n");
		return;
	}
}

//GuiInspector専用のインスペクター項目登録
void Component::SetupInspector(GuiInspector* inspector)
{
	BS_PROFILE_FUNCTION();

	if (!inspector)
	{
		OutputDebugStringA("[Component::SetupInspector] inspectorがnullです。\n");
		return;
	}
}
