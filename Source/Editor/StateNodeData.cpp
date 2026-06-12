#include "StateNodeData.h"
#include "../Serialization/JsonSerializer.h"

//ノードのパラメータをJsonSerializerにバインド
void StateNodeData::BindToSerializer(JsonSerializer* serializer)
{
	if (!serializer)return;

	serializer->RegisterVariable("State Name", &state_name);
	serializer->RegisterVariable("Animation Clip Nama", &animation_clip_name);
	serializer->RegisterVariable("Loop Animation", &is_animation_loop);
}
