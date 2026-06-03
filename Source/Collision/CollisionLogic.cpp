#include "CollisionLogic.h"

//==================================
//カプセルとスフィアの当たり判定
//=================================
bool CollisionLogic::IsCapsuleSphereCollision(
    const DirectX::XMFLOAT3& capsule_start,
    const DirectX::XMFLOAT3& capsule_end,
    float capsule_radius,
    const DirectX::XMFLOAT3& sphere_center,
    float sphere_radius)
{
    //-------------------------------------
    //DirectXMathによるベクトルへの変換
    //-------------------------------------
    DirectX::XMVECTOR vector_start = DirectX::XMLoadFloat3(&capsule_start);     //カプセルの始点ベクトルをSIMDレジスタに読み込む
    DirectX::XMVECTOR vector_end = DirectX::XMLoadFloat3(&capsule_end);         //カプセルの終点ベクトルをSIMDレジスタに読み込む
    DirectX::XMVECTOR vector_center = DirectX::XMLoadFloat3(&sphere_center);    //スフィアの中心座標ベクトルをSIMDレジスタに読み込む

    //-------------------------------------------
    //始点から終点への線分ベクトルと距離の計算
    //-------------------------------------------
    DirectX::XMVECTOR vector_line = DirectX::XMVectorSubtract(vector_end, vector_start);    //カプセルの始点から終点へ向かう線分ベクトルを計算
    DirectX::XMVECTOR dot_line_line = DirectX::XMVector3Dot(vector_line, vector_line);      //線分ベクトル同士の内積を計算し、線分の長さの２乗を求める
    float length_sq_line = DirectX::XMVectorGetX(dot_line_line);                            //計算した内積の結果を通常のfloat型として取り出す

    //----------------------------------
    //線分上の最も近い点の割合を計算
    //----------------------------------
    DirectX::XMVECTOR vector_to_center = DirectX::XMVectorSubtract(vector_center, vector_start);    //始点からスフィアの中心へ向かうベクトルを計算
    float t = 0.0f;                                                                                 //線分上のどの位置が最もスフィアに近いかを示す割合
    const float ZERO_LENGTH = 0.0f;                                                                 //ゼロ除算を防ぐための定数
    if (length_sq_line > ZERO_LENGTH)   //線分の長さが0より大きい場合
    {
        DirectX::XMVECTOR dot_line_to_center = DirectX::XMVector3Dot(vector_line, vector_center);   //線分ベクトルと、始点からスフィアへのベクトルの内積を計算
        t = DirectX::XMVectorGetX(dot_line_to_center) / length_sq_line;                             //内積を線分の長さの２乗で割り、線分上における割合を算出
        const float MIN_T = 0.0f;                                                                   //割合の下限値となる定数
        if (t < MIN_T)t = MIN_T;    //スフィアが始点より手前にある場合は割合を0に制限
        const float MAX_T = 1.0f;   //割合の上限値となる定数
        if (t > MAX_T)t = MAX_T;    //スフィアが終点より奥にある場合は割合を1に制限
    }

    //----------------------------------------------------
    //最も近い点の座標を算出し、スフィアとの距離を比較
    //----------------------------------------------------
    DirectX::XMVECTOR closest_point = DirectX::XMVectorAdd(vector_start, DirectX::XMVectorScale(vector_line, t));   //始点から線分ベクトルを割合(t)だけ進めた位置を計算
    DirectX::XMVECTOR vector_diff = DirectX::XMVectorSubtract(vector_center, closest_point);                        //最近傍点からスフィアの中心への差分ベクトルを計算
    DirectX::XMVECTOR dot_diff = DirectX::XMVector3Dot(vector_diff, vector_diff);                                   //差分ベクトル同士の内積を計算し、距離の２乗を求める
    float distance_sq = DirectX::XMVectorGetX(dot_diff);                                                            //距離の２乗を通常のfloat型として取り出す

    //---------------------------------------------
    //半径の合計と距離を比較して当たり判定を返す
    //---------------------------------------------
    float sum_radius = capsule_radius + sphere_radius;  //カプセルとスフィアの半径を合計して許容距離を求める
    float sum_radius_sq = sum_radius * sum_radius;      //平方根計算を避けるため、半径の合計も２乗
    if (distance_sq <= sum_radius_sq)   ///実際の距離の２乗が、許容される距離の２乗以下の場合は衝突
    {
        return true;    //当たっているためtrueを返す
    }
    return false;   //距離が離れているためfalseを返す
}

//スフィア同士の当たり判定
bool CollisionLogic::IsSphereSphereCollision(const SphereCollider* sphere_a, const SphereCollider* sphere_b, CollisionResult& out_result)
{
    //引数の有効判定
    if (!sphere_a || !sphere_a->is_active)return false;
    if (!sphere_b || !sphere_b->is_active)return false;

    //ベクトルへの変換
    DirectX::XMVECTOR center_a = DirectX::XMLoadFloat3(&sphere_a->center);
    DirectX::XMVECTOR center_b = DirectX::XMLoadFloat3(&sphere_b->center);

    //距離と半径を用いた交差判定
    DirectX::XMVECTOR diff_vec = DirectX::XMVectorSubtract(center_a, center_b);
    //DirectX::XMVECTOR dist_sq = DirectX::

    return false;
}
