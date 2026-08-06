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

}