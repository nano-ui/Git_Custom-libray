#include "GltfModel.h"

#include "GltfModelAnimation.h"
#include "GltfModelRenderer.h"
#include "Engine/Graphics/Resources/GltfModel/ModelRenderQueue.h"
#include "Model.h"
#include <Engine\Core\MacroTracy.h>

//コンストラクタ
GltfModel::GltfModel(const std::shared_ptr<GltfModelData>& model_data, const std::shared_ptr<GltfModelRenderer> model_renderer)
{
	BS_PROFILE_FUNCTION();

	//共有データの保持と個別コンポーネントの作成
	data = model_data;										//データの実体を共有して保持
	renderer = model_renderer;								//描画クラスの実体を共有して保持
	animation = std::make_unique<GltfModelAnimation>();		//専用のアニメーションクラスを新規作成

	//アニメーションクラスの初期化
	animation->Initialize(data);	//共有データをアニメーションクラスに渡し、初期姿勢などを構築
}

//デストラクタ
GltfModel::~GltfModel()
{
}

//更新処理
void GltfModel::Update(float delta_time)
{
	BS_PROFILE_FUNCTION();

	//アニメーションの時間を進めて計算を実行
	if (animation)	//アニメーションクラスが存在しているか確認
	{
		animation->UpdateAnimation(delta_time);	//経過時間だけを渡して、自動計算
	}
}

//描画処理
void GltfModel::Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world_matrix)
{
	BS_PROFILE_FUNCTION();

	//描画クラスにデータと計算結果を渡してGPUへ命令
	if (renderer && data && animation)	//要な3つのコンポーネントがすべて揃っているか確認
	{
		//自身の共有データ、指定された位置（ワールド行列）、自身の計算済みノード配列を渡して描画を実行
		renderer->Render(immediate_context, *this, world_matrix, nullptr);
	}
}

//レンダーキューに登録
void GltfModel::SubmitRenderCommands(ModelRenderQueue& queue, const DirectX::XMFLOAT4X4& world_matrix)
{
	BS_PROFILE_FUNCTION();

	if (!data)
	{
		OutputDebugStringA("[GltfModel エラー] SubmitRenderCommands: data が nullptr です。\n");
		return;
	}

	if (!animation)
	{
		OutputDebugStringA("[GltfModel エラー] SubmitRenderCommands: animation が nullptr です。\n");
		return;
	}

	const std::vector<GltfModelData::node>& nodes = animation->GetAnimationNodes();
	if (nodes.empty())
	{
		OutputDebugStringA("[GltfModel 警告] SubmitRenderCommands: nodes が空です。\n");
		return;
	}

	DirectX::XMMATRIX base_world = DirectX::XMLoadFloat4x4(&world_matrix);

	for (const GltfModelData::cached_command& cmd : data->cached_render_commands)
	{
		if (cmd.node_index < 0 || static_cast<size_t>(cmd.node_index) >= nodes.size())
		{
			OutputDebugStringA("[GltfModel エラー] SubmitRenderCommands: 不正な node_index です。\n");
			continue;
		}

		if (cmd.mesh_index < 0 || static_cast<size_t>(cmd.mesh_index) >= data->meshes.size())
		{
			OutputDebugStringA("[GltfModel エラー] SubmitRenderCommands: 不正な mesh_index です。\n");
			continue;
		}

		const GltfModelData::mesh& mesh = data->meshes.at(cmd.mesh_index);
		if (cmd.primitive_index < 0 || static_cast<size_t>(cmd.primitive_index) >= mesh.primitives.size())
		{
			OutputDebugStringA("[GltfModel エラー] SubmitRenderCommands: 不正な primitive_index です。\n");
			continue;
		}

		const GltfModelData::node& current_node = nodes.at(cmd.node_index);
		const GltfModelData::mesh::primitive* primitive = &mesh.primitives.at(cmd.primitive_index);

		ModelDrawCommand draw_command = {};
		draw_command.data = data.get();
		draw_command.nodes = &nodes;
		draw_command.primitive = primitive;
		draw_command.material_index = cmd.material_index;
		draw_command.skin_index = current_node.skin;
		draw_command.node_index = cmd.node_index;

		DirectX::XMMATRIX node_global = DirectX::XMLoadFloat4x4(&current_node.global_transform);
		DirectX::XMMATRIX final_world = DirectX::XMMatrixMultiply(node_global, base_world);
		DirectX::XMStoreFloat4x4(&draw_command.world_matrix, final_world);

		queue.Submit(draw_command);
	}
}

//アニメーションの再生切り替え
void GltfModel::PlayAnimation(const std::string& animation_name, bool is_loop)
{
	BS_PROFILE_FUNCTION();

	//--------------------------------------
	//アニメーションクラスへの命令の委譲
	//--------------------------------------
	if (animation)	//アニメーションクラスが存在しているか確認
	{
		animation->PlayAniamtion(animation_name, is_loop);	//名前とループ設定をそのまま中継
	}
}

//アニメーションの終了したか取得
bool GltfModel::IsAnimationFinished() const
{
	BS_PROFILE_FUNCTION();

	if (animation)
	{
		return animation->IsFinished();
	}

	return true;
}

//アニメーション情報取得
const std::vector<GltfModelData::node>& GltfModel::GetAnimatedNodes() const
{
	BS_PROFILE_FUNCTION();

	return animation->GetAnimationNodes();
}

//現在の再生経過時間を取得
float GltfModel::GetAnimationCurrentTime() const
{
	BS_PROFILE_FUNCTION();

	return animation->GetCurrentAnimationTime();
}

//再生時間を設定
void GltfModel::SetAnimationTime(float time)
{
	BS_PROFILE_FUNCTION();

	if (animation)
	{
		animation->SetCurrentAnimationTime(time);
	}
}

//アニメーションの総時間を取得
float GltfModel::GetAnimationDuration() const
{
	BS_PROFILE_FUNCTION();

	return animation->GetAnimationDuration();
}

//指定したノードの移動成分を外部から上書き
void GltfModel::SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation)
{
	BS_PROFILE_FUNCTION();

	if (animation)
	{
		animation->SetNodeTranslation(node_index, translation);
	}
}

//変更されたノード情報をもとに、グローバル行列を再計算
void GltfModel::RecalculateTransforms()
{
	BS_PROFILE_FUNCTION();

	if (animation)
	{
		animation->CumulateTransforms();
	}
}

//アニメーションブレンド
void GltfModel::AnimateBlend(const std::string& anim_a, float time_a, const std::string& anim_b, float time_b, float blend_factor)
{
	BS_PROFILE_FUNCTION();

	if (animation && data)
	{
		int index_a = data->GetAnimationIndex(anim_a.c_str());
		int index_b = data->GetAnimationIndex(anim_b.c_str());
		if (index_a >= 0 && index_b >= 0.0f)animation->AnimationBlend(static_cast<size_t>(index_a), time_a, static_cast<size_t>(index_b), time_b, blend_factor);
	}
}

//ノード名からインデックスを検索
int GltfModel::FindNodeIndex(const std::string& node_name) const
{
	BS_PROFILE_FUNCTION();

	if (node_name.empty())
	{
		printf_s("------------------------------------------------------\n");
		printf_s("[GltfModel 警告] FindNodeIndex: ノード名が空です。\n");
		printf_s("------------------------------------------------------\n");
		return -1;
	}

	const std::vector<GltfModelData::node>& nodes = GetAnimatedNodes();
	for (size_t i = 0; i < nodes.size(); i++)
	{
		if (nodes[i].name == node_name)
		{
			return static_cast<int>(i);
		}
	}

	printf_s("------------------------------------------------------\n");
	std::string debug_msg = "[GltfModel 警告] FindNodeIndex: ノード '" + node_name + "' が見つかりませんでした。\n";
	printf_s(debug_msg.c_str());
	printf_s("------------------------------------------------------\n");

	return -1;
}

//ノード名からモデル空間のグローバル行列を取得
bool GltfModel::GetNodeGlobalTransform(const std::string& node_name, DirectX::XMFLOAT4X4& out_transform) const
{
	BS_PROFILE_FUNCTION();

	int index = FindNodeIndex(node_name);
	if (index < 0)return false;
	return GetNodeGlobalTransform(index, out_transform);
}

//ノードインデックスからモデル空間のグローバル行列を取得
bool GltfModel::GetNodeGlobalTransform(int node_index, DirectX::XMFLOAT4X4& out_transform) const
{
	BS_PROFILE_FUNCTION();

	const std::vector<GltfModelData::node>& nodes = GetAnimatedNodes();
	if (node_index < 0 || static_cast<size_t>(node_index) >= nodes.size())
	{
		printf_s("------------------------------------------------------\n");
		printf_s("[GltfModel エラー] GetNodeGlobalTransform: 指定されたインデックスが範囲外です。\n");
		printf_s("------------------------------------------------------\n");
		return false;
	}
	
	out_transform = nodes[static_cast<size_t>(node_index)].global_transform;
	return true;
}
