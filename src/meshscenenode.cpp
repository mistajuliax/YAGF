// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/MeshSceneNode.h>
#include <Scene/textures.h>
#include <assimp/Importer.hpp>
#include <Maths/matrix4.h>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#ifdef D3D12
#define SAMPLE_PATH ""
#else
#define SAMPLE_PATH "..\\..\\..\\examples\\assets\\"
#endif


struct ObjectData
{
	float ModelMatrix[16];
	float InverseModelMatrix[16];
};

namespace irr
{
	namespace scene
	{
		//! Constructor
		/** Use setMesh() to set the mesh to display.
		*/
		IMeshSceneNode::IMeshSceneNode(device_t dev, const aiScene*model, command_list_t upload_cmd_list, descriptor_storage_t heap, vulkan_wrapper::pipeline_descriptor_set* object_set, vulkan_wrapper::pipeline_descriptor_set* model_set,
			ISceneNode* parent,
			const core::vector3df& position,
			const core::vector3df& rotation,
			const core::vector3df& scale)
			: ISceneNode(parent, position, rotation, scale)
		{
			object_matrix = create_buffer(dev, sizeof(ObjectData));
			object_descriptor_set = util::allocate_descriptor_sets(dev->object, heap->object, { object_set->object });

			util::update_descriptor_sets(dev->object,
			{
				structures::write_descriptor_set(object_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				{ VkDescriptorBufferInfo{ object_matrix->object, 0, sizeof(ObjectData) } }, 0)
				/*		structures::write_descriptor_set(object_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				{ VkDescriptorBufferInfo{ jointbuffer->object, 0, sizeof(JointTransform) } }, 1),*/
			});

			// Format Weight

			/*        std::vector<std::vector<irr::video::SkinnedVertexData> > weightsList;
			for (auto weightbuffer : loader->AnimatedMesh.WeightBuffers)
			{
			std::vector<irr::video::SkinnedVertexData> weights;
			for (unsigned j = 0; j < weightbuffer.size(); j += 4)
			{
			irr::video::SkinnedVertexData tmp = {
			weightbuffer[j].Index, weightbuffer[j].Weight,
			weightbuffer[j + 1].Index, weightbuffer[j + 1].Weight,
			weightbuffer[j + 2].Index, weightbuffer[j + 2].Weight,
			weightbuffer[j + 3].Index, weightbuffer[j + 3].Weight,
			};
			weights.push_back(tmp);
			}
			weightsList.push_back(weights);
			}*/

			size_t total_vertex_cnt = 0;
			total_index_cnt = 0;
			for (int i = 0; i < model->mNumMeshes; ++i)
			{
				const aiMesh *mesh = model->mMeshes[i];
				total_vertex_cnt += mesh->mNumVertices;
				total_index_cnt += mesh->mNumFaces * 3;
			}

			index_buffer = create_buffer(dev, total_index_cnt * sizeof(uint16_t));
			uint16_t *indexmap = (uint16_t *)map_buffer(dev, index_buffer);
			vertex_pos = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
			aiVector3D *vertex_pos_map = (aiVector3D*)map_buffer(dev, vertex_pos);
			vertex_normal = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
			aiVector3D *vertex_normal_map = (aiVector3D*)map_buffer(dev, vertex_normal);
			vertex_uv0 = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
			aiVector3D *vertex_uv_map = (aiVector3D*)map_buffer(dev, vertex_uv0);

			size_t basevertex = 0, baseindex = 0;

			for (int i = 0; i < model->mNumMeshes; ++i)
			{
				const aiMesh *mesh = model->mMeshes[i];
				for (int vtx = 0; vtx < mesh->mNumVertices; ++vtx)
				{
					vertex_pos_map[basevertex + vtx] = mesh->mVertices[vtx];
					vertex_normal_map[basevertex + vtx] = mesh->mNormals[vtx];
					vertex_uv_map[basevertex + vtx] = mesh->mTextureCoords[0][vtx];
				}
				for (int idx = 0; idx < mesh->mNumFaces; ++idx)
				{
					indexmap[baseindex + 3 * idx] = mesh->mFaces[idx].mIndices[0];
					indexmap[baseindex + 3 * idx + 1] = mesh->mFaces[idx].mIndices[1];
					indexmap[baseindex + 3 * idx + 2] = mesh->mFaces[idx].mIndices[2];
				}
				meshOffset.push_back(std::make_tuple(mesh->mNumFaces * 3, basevertex, baseindex));
				basevertex += mesh->mNumVertices;
				baseindex += mesh->mNumFaces * 3;
				texture_mapping.push_back(mesh->mMaterialIndex);
			}

			unmap_buffer(dev, index_buffer);
			unmap_buffer(dev, vertex_pos);
			unmap_buffer(dev, vertex_normal);
			unmap_buffer(dev, vertex_uv0);
			// TODO: Upload to GPUmem

			vertex_buffers_info.emplace_back(vertex_pos, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
			vertex_buffers_info.emplace_back(vertex_normal, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
			vertex_buffers_info.emplace_back(vertex_uv0, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));

			// Texture
			for (int texture_id = 0; texture_id < model->mNumMaterials; ++texture_id)
			{
				aiString path;
				model->mMaterials[texture_id]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
				std::string texture_path(path.C_Str());
				const std::string &fixed = SAMPLE_PATH + texture_path.substr(0, texture_path.find_last_of('.')) + ".DDS";

				image_t texture;
				buffer_t upload_buffer;
				std::tie(texture, upload_buffer) = load_texture(dev, fixed, upload_cmd_list);
				Textures.push_back(texture);
				upload_buffers.push_back(upload_buffer);
#ifdef D3D12
				create_image_view(dev, cbv_srv_descriptors_heap, 8 + texture_id, texture, 9, irr::video::ECOLOR_FORMAT::ECF_BC1_UNORM_SRGB, D3D12_SRV_DIMENSION_TEXTURE2D);
#else
				VkDescriptorSet mesh_descriptor = util::allocate_descriptor_sets(dev->object, heap->object, { model_set->object });
				mesh_descriptor_set.push_back(mesh_descriptor);
				auto img_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, texture->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
					structures::component_mapping(), structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, 0, texture->info.mipLevels));
				Textures_views.push_back(img_view);
				util::update_descriptor_sets(dev->object,
				{
					structures::write_descriptor_set(mesh_descriptor, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,{ VkDescriptorImageInfo{ VK_NULL_HANDLE, img_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 2)
				});
#endif
			}
		}

		IMeshSceneNode::~IMeshSceneNode()
		{

		}


		static float timer = 0.;

		void IMeshSceneNode::fill_draw_command(device_t dev, command_list_t current_cmd_list, pipeline_layout_t object_sig)
		{
#ifdef D3D12
			current_cmd_list->SetGraphicsRootDescriptorTable(1,
				CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart())
				.Offset(3, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#else
			vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 1, 1, &object_descriptor_set, 0, nullptr);
#endif // D3D12

			bind_index_buffer(current_cmd_list, index_buffer, 0, total_index_cnt * sizeof(uint16_t), irr::video::E_INDEX_TYPE::EIT_16BIT);
			bind_vertex_buffers(current_cmd_list, 0, vertex_buffers_info);

			for (unsigned i = 0; i < meshOffset.size(); i++)
			{
#ifdef D3D12
				current_cmd_list->SetGraphicsRootDescriptorTable(0,
					CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart())
					.Offset(texture_mapping[i] + 8, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#else
				vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 0, 1, &mesh_descriptor_set[texture_mapping[i]], 0, nullptr);
#endif
				draw_indexed(current_cmd_list, std::get<0>(meshOffset[i]), 1, std::get<2>(meshOffset[i]), std::get<1>(meshOffset[i]), 0);
			}
		}

		void IMeshSceneNode::update_constant_buffers(device_t dev)
		{
			ObjectData *cbufdata = static_cast<ObjectData*>(map_buffer(dev, object_matrix));
			irr::core::matrix4 Model;
			irr::core::matrix4 InvModel;
			Model.setTranslation(irr::core::vector3df(0.f, 0.f, 2.f));
			Model.setRotationDegrees(irr::core::vector3df(0.f, timer / 360.f, 0.f));
			Model.getInverse(InvModel);

			memcpy(cbufdata->ModelMatrix, Model.pointer(), 16 * sizeof(float));
			memcpy(cbufdata->InverseModelMatrix, InvModel.pointer(), 16 * sizeof(float));
			unmap_buffer(dev, object_matrix);

			timer += 16.f;
		}

	}
}