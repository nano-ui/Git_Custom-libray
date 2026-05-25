#include "GltfModel.h"

#include "GltfModelData.h"
#include "GltfModelAnimation.h"
#include "GltfModelRenderer.h"
#include "Model.h"

//==================
//コンストラクタ
//==================
GltfModel::GltfModel(const std::shared_ptr<GltfModelData>& model_data, const std::shared_ptr<GltfModelRenderer> model_renderer)
{
	//---------------------------------------------
	//共有データの保持と個別コンポーネントの作成
	//---------------------------------------------
	data = model_data;										//データの実体を共有して保持
	renderer = model_renderer;								//描画クラスの実体を共有して保持
	animation = std::make_unique<GltfModelAnimation>();		//専用のアニメーションクラスを新規作成

	//-------------------------------
	//アニメーションクラスの初期化
	//------------------------------
	animation->Initialize(data);	//共有データをアニメーションクラスに渡し、初期姿勢などを構築
}

//================
//デストラクタ
//=================
GltfModel::~GltfModel()
{
}

//===========
//更新処理
//===========
void GltfModel::Update(float delta_time)
{
	//-------------------------------------------
	//アニメーションの時間を進めて計算を実行
	//-------------------------------------------
	if (animation)	//アニメーションクラスが存在しているか確認
	{
		animation->UpdateAnimation(delta_time);	//経過時間だけを渡して、自動計算
	}
}

//===========
//描画処理
//===========
void GltfModel::Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world_matrix)
{
	//------------------------------------------------
	//描画クラスにデータと計算結果を渡してGPUへ命令
	//------------------------------------------------
	if (renderer && data && animation)	//要な3つのコンポーネントがすべて揃っているか確認
	{
		//自身の共有データ、指定された位置（ワールド行列）、自身の計算済みノード配列を渡して描画を実行
		renderer->Render(immediate_context, *data, world_matrix, animation->GetAnimationNodes());
	}
}

//===============================
//アニメーションの再生切り替え
//===============================
void GltfModel::PlayAnimation(const std::string& animation_name, bool is_loop)
{
	//--------------------------------------
	//アニメーションクラスへの命令の委譲
	//--------------------------------------
	if (animation)	//アニメーションクラスが存在しているか確認
	{
		animation->PlayAniamtion(animation_name, is_loop);	//名前とループ設定をそのまま中継
	}
}
