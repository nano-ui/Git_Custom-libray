#include "DirectionJudgment.h"
#include "DirectionJudgment.h"

//=======================
//コンストラクタ
//=======================
DirectionJudgment::DirectionJudgment(
    const DirectX::XMFLOAT3& pos_ref,
    const DirectX::XMFLOAT3& front_ref,
    const DirectX::XMFLOAT3& target_pos_ref)
    : pos_ref(pos_ref)         // 位置参照の初期化
    , front_ref(front_ref)     // 前方ベクトル参照の初期化
    , target_pos_ref(target_pos_ref) // 対象位置参照の初期化
{
}

//==============================
//相対方向を取得する処理
//==============================
RelativeDirection DirectionJudgment::GetRelativeDirection()
{
    //-------------------------------
    //データの取得
    //-------------------------------
    DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&pos_ref.get());        // 自身の座標
    DirectX::XMVECTOR front = DirectX::XMLoadFloat3(&front_ref.get());          // 自身の前方
    DirectX::XMVECTOR target_pos = DirectX::XMLoadFloat3(&target_pos_ref.get()); // 対象の座標

    //---------------------------
    //対象の方向ベクトルを計算
    //---------------------------
    DirectX::XMVECTOR to_target = DirectX::XMVectorSubtract(target_pos, position);  //ターゲットへのベクトルを算出
    to_target = DirectX::XMVectorSetY(to_target, 0.0f); //Y成分を無効化(平面化で判定)
    front = DirectX::XMVectorSetY(front, 0.0f);         //前方ベクトルのY成分を無効化

    //---------------------------
    //ゼロベクトルチェック
    //---------------------------
    DirectX::XMVECTOR length_sq = DirectX::XMVector3LengthSq(to_target);          //距離の2乗を計算
    DirectX::XMVECTOR epsilon = DirectX::XMVectorReplicate(zero_tolerance);       //誤差値のレプリケート
    if (DirectX::XMVector3LessOrEqual(length_sq, epsilon))                        //距離がほぼ0かチェック
    {
        return RelativeDirection::None; // 判定不可を返す
    }

    //-----------------------
    //ベクトルの正規化
    //-----------------------
    to_target = DirectX::XMVector3Normalize(to_target); //ターゲット方向を単位ベクトルで正規化
    front = DirectX::XMVector3Normalize(front);         //前方方句を単位ベクトルで正規化

    //-------------------------
    //内積を用いた前後判定
    //-------------------------
    DirectX::XMVECTOR dot_v = DirectX::XMVector3Dot(front, to_target);  //内積の計算
    float dot;  //内積を格納用
    DirectX::XMStoreFloat(&dot, dot_v); //内積の結果を格納

    //------------------------
    //外積を用いた左右判定
    //------------------------
    DirectX::XMVECTOR cross_v = DirectX::XMVector3Cross(front, to_target);  //外積の計算
    float cross_y = DirectX::XMVectorGetY(cross_v); //Y成分（回転軸）を取得

    //---------------------------
    //角度に基づく方向の確定
    //---------------------------
    if (dot > threshold_front) // 22.5度以内
    {
        return RelativeDirection::Front; // 前方
    }

    if (dot > threshold_side) // 22.5度 ～ 67.5度
    {
        return (cross_y > 0.0f) ? RelativeDirection::FrontRight : RelativeDirection::FrontLeft; // 前方斜め
    }

    if (dot > threshold_back) // 67.5度 ～ 112.5度
    {
        return (cross_y > 0.0f) ? RelativeDirection::Right : RelativeDirection::Left; // 真横
    }

    if (dot > threshold_rear) // 112.5度 ～ 157.5度
    {
        return (cross_y > 0.0f) ? RelativeDirection::BackRight : RelativeDirection::BackLeft; // 後方斜め
    }

    return RelativeDirection::Back;
}

//================
//仮の判定処理
//================
bool DirectionJudgment::Check()
{
    RelativeDirection current_dir = GetRelativeDirection();
    return (current_dir != RelativeDirection::None);
}
