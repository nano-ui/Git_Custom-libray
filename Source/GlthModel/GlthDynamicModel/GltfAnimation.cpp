#include "GltfAnimation.h"
#include "GltfBone.h"

#include <tiny_gltf.h>
#include <algorithm>

//=========================================
//�R���X�g���N�^
//=========================================
GltfAnimation::GltfAnimation()
{
	current_time = 0.0f;
	max_duration = 0.0f;
}

//=========================================
//�A�j���[�V�����̏�����
//=========================================
void GltfAnimation::Initialize(const tinygltf::Model& model, const tinygltf::Animation& anim)
{
	//---------------------------
	//��{���̓ǂݍ���
	//---------------------------

	//�A�j���[�V���������R�s�[
	animation_name = anim.name;
	//���������m��
	channels.reserve(anim.channels.size());

	//---------------------------
	//�S�`�����l�������[�v���ĉ��
	//---------------------------

	for (const auto& gltf_channel : anim.channels)
	{
		Channel channel = {};
		//�Ώۂ̃m�[�h�ԍ����Z�b�g
		channel.target_bone_index = gltf_channel.target_node;
		channel.last_key_index = 0;	//�����q���g��������

		//�^�[�Q�b�g���ڂ̔���
		if (gltf_channel.target_path == "translation")
		{
			//�ʒu�ړ�
			channel.target_type = TargetType::Translation;
		}
		else if (gltf_channel.target_path == "rotation")
			//��]
			channel.target_type = TargetType::Rotation;
		}
		else if (gltf_channel.target_path == "scale")
		{
			//�g��E�k��
			channel.target_type = TargetType::Scale;
		}

		//�T���v���[�̎擾
		const auto& sampler = anim.samplers[gltf_channel.sampler];

		//���Ԏ��f�[�^�̃|�C���^�擾
		const auto& input_acc = model.accessors[sampler.input];		//�A�N�Z�b�T���擾
		const float* time_ptr = reinterpret_cast<const float*>(&model.buffers[model.bufferViews[input_acc.bufferView].buffer].data[model.bufferViews[input_acc.bufferView].byteOffset + input_acc.byteOffset]);//���Ԃ̐擪
		//�l�f�[�^�̃|�C���^�擾
		const auto& output_acc = model.accessors[sampler.output];	//�A�N�Z�b�T���擾
		const float* val_ptr = reinterpret_cast<const float*>(&model.buffers[model.bufferViews[output_acc.bufferView].buffer].data[model.bufferViews[output_acc.bufferView].byteOffset + output_acc.byteOffset]); //�l�f�[�^�擪

		//�L�[�t���[���̍\�z
		channel.keyframes.reserve(input_acc.count);	//�������m��

		for (size_t i = 0; i < input_acc.count; i++)
		{
			//�L�[�t���[��
			Keyframe kf = {};
			kf.time = time_ptr[i];	//���ԃZ�b�g
			if (max_duration < kf.time)
			{
				//�ő厞�ԍX�V
				max_duration = kf.time;
			}
			//��]�̏ꍇ
			if (channel.target_type == TargetType::Rotation)
			{
				kf.value = { val_ptr[i * 4], val_ptr[i * 4 + 1], val_ptr[i * 4 + 2], val_ptr[i * 4 + 3] };
			}
			//�ʒu�E�X�P�[���̏ꍇ
			else
			{
				kf.value = { val_ptr[i * 3], val_ptr[i * 3 + 1], val_ptr[i * 3 + 2], 0.0f };
			}
			channel.keyframes.push_back(kf);//���X�g�ɒǉ�
		}
		//�`�����l���o�^
		channels.push_back(channel);
	}
}

//=========================================
//�A�j���[�V�����̍X�V
//=========================================
void GltfAnimation::Update(float delta_time, std::vector<GltfBone>& bones)
{
	//---------------------------
	//���Ԃ̍X�V
	//---------------------------

	current_time += delta_time;	//�o�ߎ��Ԃ����Z
	if (current_time > max_duration)
	{
		current_time = fmod(current_time, max_duration);
	}

	//---------------------------
	//�S�`�����l���̕ω����{�[���ɓK�p
	//---------------------------

	//�o�^���ꂽ�S�`�����l�������[�v
	for (const auto& channel : channels)
	{
		//�Ώۃ{�[����������΃X�L�b�v
		if (channel.target_bone_index < 0) continue;

		//---------------------------
		//���݂̎��ԂɊY������L�[�t���[����T��
		//---------------------------

		size_t next_index = 0;//���̃L�[�t���[���̃C���f�b�N�X

		//�L�[�t���[����񕪒T���Ō���
		auto it = std::upper_bound(channel.keyframes.begin(), channel.keyframes.end(), current_time,
			[](float t, const Keyframe& k) { return t < k.time; });

		//���̃C���f�b�N�X
		next_index = std::distance(channel.keyframes.begin(), it);

		if (next_index == 0) next_index = 1;//�ŏ��C���f�b�N�X��ۏ�
		size_t prev_index = next_index - 1;	//1�O�����݂̃t���[���ɂ���

		//---------------------------
		//�⊮�W�������߂�
		//---------------------------

		float prev_time = channel.keyframes[prev_index].time;	//�O�̎���
		float next_time = channel.keyframes[next_index].time;	//���̎���
		float time = (prev_time == next_time) ? 0.0f : (current_time - prev_time) / (next_time - prev_time);//0.0�`1.0�̊������v�Z

		//---------------------------
		//�l�̕⊮�Ɣ��f
		//---------------------------

		auto& bone = bones[channel.target_bone_index];	//���������Ώۂ̃{�[���Q��
		DirectX::XMMATRIX bone_local = DirectX::XMLoadFloat4x4(&bone.GetLocalMatrix());//���݂̍s����擾
		DirectX::XMVECTOR v_pos, v_rot, v_scl;//TRS����p�̃x�N�g��
		DirectX::XMMatrixDecompose(&v_scl, &v_rot, &v_pos, bone_local);//�s���TRS�ɕ���

		//---------------------------
		//�`�����l���̎�ނɉ����āA������������������������
		//---------------------------

		if (channel.target_type == TargetType::Translation)
		{
			//�ʒu
			v_pos = DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&channel.keyframes[prev_index].value), DirectX::XMLoadFloat4(&channel.keyframes[next_index].value), time);
		}
		else if (channel.target_type == TargetType::Rotation)
		{
			//��]
			v_rot = DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&channel.keyframes[prev_index].value), DirectX::XMLoadFloat4(&channel.keyframes[next_index].value), time);
		}
		else if (channel.target_type == TargetType::Scale)
		{
			//�X�P�[��
			v_scl = DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&channel.keyframes[prev_index].value), DirectX::XMLoadFloat4(&channel.keyframes[next_index].value), time);
		}

		//---------------------------
		//�s��̍č����ƓK�p
		//---------------------------
		
		DirectX::XMMATRIX new_local = DirectX::XMMatrixAffineTransformation(v_scl, DirectX::XMQuaternionIdentity(), v_rot, v_pos);//����
		DirectX::XMFLOAT4X4 stored_matrix;	//�ۑ��p
		DirectX::XMStoreFloat4x4(&stored_matrix, new_local);//XMFLOAT4X4�ɕϊ�
		bone.SetLocalMatrix(stored_matrix);	//�{�[���ɏ����߂�
	}
}
