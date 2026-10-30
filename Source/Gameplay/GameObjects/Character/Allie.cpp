#include "Allie.h"
#include "Gameplay\GameObjects\ObjectFactory.h"
#include "Engine\Core\Log\Log.h"

static AutoRegister<Allie> auto_register_class("Allie");
//コンストラクタ
Allie::Allie()
{
	Initialize();
}

//デストラクタ
Allie::~Allie() = default;

//初期化処理
void Allie::Initialize()
{
	Character::Initialize();
	SetupComponent();
	target_position = { 0.0f,0.0f,0.0f };
	has_target = false;
}

//更新処理
void Allie::Update(float elapsed_time)
{
	Character::Update(elapsed_time);
}

//衝突処理
void Allie::OnCollisionHit(const CollisionResult& result)
{

}

//描画処理
void Allie::Render(ID3D11DeviceContext* context)
{

}

//デバッグ描画
void Allie::RenderDebug(ShapeRenderer* renderer)
{

}

// シリアライズ登録
void Allie::SetupSerialization()
{

}

//インスペクター登録
void Allie::SetupInspector()
{

}

//コンポーネントの設定
void Allie::SetupComponent()
{

}

//デフォルトパスの取得
std::string Allie::GetDefaultModelPath() const
{
	LOG_NORMAL({ u8"設定" }, u8"デフォルトパスが設定されました");
	return "Data/Model/Character/Player/Greystone_WhiteTiger.gltf";
}


