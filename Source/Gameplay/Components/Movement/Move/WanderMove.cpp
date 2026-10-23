#include "WanderMove.h"
#include "LinearMove.h"
#include "Engine\Core\Log\Log.h"

//コンストラクタ
WanderMove::WanderMove()
{
	liner_move = std::make_unique<LinearMove>(0);
	home_position = { 0.0f,0.0f,0.0f };
	wander_radius = 30.0f;
	target_pos = { 0.0f,0.0f,0.0f };
	wait_duration = 1.0f;
	wait_timer = 0.0f;
	is_wanting = true;
}

//デストラクタ
WanderMove::~WanderMove() = default;

//更新処理
DirectX::XMFLOAT3 WanderMove::Update(
	const DirectX::XMFLOAT3& current_pos,
	const DirectX::XMFLOAT3& target_pos,
	const StateBlackboard* blackboard)
{
	return DirectX::XMFLOAT3();
}

//徘徊処理
DirectX::XMFLOAT3 WanderMove::UpdateWander(
	float elapsed_time, 
	const DirectX::XMFLOAT3& current_pos,
	float speed)
{
	//待機中の処理
	if (is_wanting)
	{
		wait_timer += elapsed_time;
		if (wait_timer >= wait_duration)
		{
			wait_timer = 0.0f;
			is_wanting = false;

			//-1.0f ～ 1.0f の乱数を生成
			float value_x = (static_cast<float>(rand()) / RAND_MAX * 2.0f) - 1.0f;
			float value_z = (static_cast<float>(rand()) / RAND_MAX * 2.0f) - 1.0f;

			//次の目的地を設定
			target_pos = {
				home_position.x + (wander_radius * value_x),
				home_position.y,
				home_position.z + (wander_radius * value_z),
			};
		}
		return { 0.0f,0.0f,0.0f };
	}

	//到着判定
	static constexpr float margin = 0.5f;
	float length = 0.0f;
	DirectX::XMVECTOR start_vector = DirectX::XMLoadFloat3(&current_pos);	
	DirectX::XMVECTOR end_vector = DirectX::XMLoadFloat3(&target_pos);		
	DirectX::XMVECTOR diff_vector = DirectX::XMVectorSubtract(end_vector, start_vector);	
	DirectX::XMVECTOR length_vector = DirectX::XMVector3Length(diff_vector);			
	DirectX::XMStoreFloat(&length, length_vector);
	
	//目的地に到着したら待機へ切り替え
	if (length <= margin)
	{
		is_wanting = true;
		return { 0.0f,0.0f,0.0f };
	}

	//目的地へ移動
	return 	liner_move->UpdateLiner(current_pos, target_pos, speed);
}
