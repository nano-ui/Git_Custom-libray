#include "sky_box.hlsli"

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;
	
	//ローカル座標の保存とワールド変換
    vout.local_position = vin.position;
    float4 world_pos = mul(float4(vin.position, 1.0f), world);
	
	//カメラ位置への追従処理
    world_pos.xyz += camera_position.xyz;
	
	//画面座標への変換と深度値の書き換え
    vout.position = mul(world_pos, view_projection);
    vout.position.z = vout.position.w;
	
    return vout;
}