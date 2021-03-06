#include "mesh.h"
#include <Scene\IBL.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <gflags/gflags.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define CHECK_HRESULT(cmd)                                                     \
  {                                                                            \
    HRESULT hr = cmd;                                                          \
    if (hr != 0)                                                               \
      throw;                                                                   \
  }

#define SAMPLE_PATH "..\\..\\..\\examples\\assets\\"

struct SceneData {
  glm::mat4 ViewMatrix;
  glm::mat4 InverseViewMatrix;
  glm::mat4 ProjectionMatrix;
  glm::mat4 InverseProjectionMatrix;
};

/*struct JointTransform
{
        float Model[16 * 48];
};*/

namespace {
// (inv)modelmatrix, jointmatrix
const auto object_descriptor_set_type = descriptor_set(
    {range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
     range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 1, 1)},
    shader_stage::all);

const auto model_descriptor_set_type =
    descriptor_set({range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 2, 1)},
                   shader_stage::fragment_shader);

// anisotropic, bilinear
const auto sampler_descriptor_set_type =
    descriptor_set({range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1),
                    range_of_descriptors(RESOURCE_VIEW::SAMPLER, 13, 1)},
                   shader_stage::fragment_shader);

// color, normal, roughness+metalness, depth
const auto input_attachments_descriptor_set_type = descriptor_set(
    {range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 4, 1),
     range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 5, 1),
     range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 6, 1),
     range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 14, 1)},
    shader_stage::fragment_shader);

// color, normal,roughness+metalness, depth, ssao
const auto rtt_descriptor_set_type = descriptor_set(
    {range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 4, 1),
     range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 5, 1),
     range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 6, 1),
     range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 14, 1),
     range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 15, 1)},
    shader_stage::fragment_shader);

// view/proj matrixes, sunlight data, skybox
const auto scene_descriptor_set_type =
    descriptor_set({range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 7, 1),
                    range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 8, 1),
                    range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 9, 1)},
                   shader_stage::all);

// IBL data
const auto ibl_descriptor_set_type = descriptor_set(
    {range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 10, 1),
     range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 11, 1),
     range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 12, 1)},
    shader_stage::fragment_shader);
}

void MeshSample::Init() {
  back_buffer = chain->get_image_view_from_swap_chain();

  command_allocator = dev->create_command_storage();
  auto command_list = command_allocator->create_command_list();
  command_list->start_command_list_recording(*command_allocator);

  cbv_srv_descriptors_heap = dev->create_descriptor_storage(
      100, {{RESOURCE_VIEW::CONSTANTS_BUFFER, 10},
            {RESOURCE_VIEW::SHADER_RESOURCE, 1000},
            {RESOURCE_VIEW::INPUT_ATTACHMENT, 4},
            {RESOURCE_VIEW::UAV_BUFFER, 1}});
  sampler_heap =
      dev->create_descriptor_storage(10, {{RESOURCE_VIEW::SAMPLER, 10}});
  object_sunlight_pass = dev->create_object_sunlight_pass(swap_chain_format);
  ibl_skyboss_pass = dev->create_ibl_sky_pass(swap_chain_format);

  load_program_and_pipeline_layout();

  scene_matrix = dev->create_buffer(
      sizeof(SceneData), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE,
      usage_uniform);
  sun_data = dev->create_buffer(7 * sizeof(float),
                                irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE,
                                usage_uniform);

  clear_value_t clear_val = get_clear_value(irr::video::D24U8, 1., 0);
#ifndef D3D12
  command_list->set_pipeline_barrier(*back_buffer[0], RESOURCE_USAGE::undefined,
                                     RESOURCE_USAGE::PRESENT, 0,
                                     irr::video::E_ASPECT::EA_COLOR);
  command_list->set_pipeline_barrier(*back_buffer[1], RESOURCE_USAGE::undefined,
                                     RESOURCE_USAGE::PRESENT, 0,
                                     irr::video::E_ASPECT::EA_COLOR);
#endif // !D3D12
  createTextures();
  std::unique_ptr<buffer_t> upload_buffer;
  std::tie(skybox_texture, upload_buffer) = load_texture(
      *dev, SAMPLE_PATH + std::string("w_sky_1BC1.DDS"), *command_list);
  command_list->set_pipeline_barrier(*depth_buffer, RESOURCE_USAGE::undefined,
                                     RESOURCE_USAGE::DEPTH_WRITE, 0,
                                     irr::video::E_ASPECT::EA_DEPTH_STENCIL);
  command_list->set_pipeline_barrier(*diffuse_color, RESOURCE_USAGE::undefined,
                                     RESOURCE_USAGE::RENDER_TARGET, 0,
                                     irr::video::E_ASPECT::EA_COLOR);
  command_list->set_pipeline_barrier(*normal, RESOURCE_USAGE::undefined,
                                     RESOURCE_USAGE::RENDER_TARGET, 0,
                                     irr::video::E_ASPECT::EA_COLOR);
  command_list->set_pipeline_barrier(
      *roughness_metalness, RESOURCE_USAGE::undefined,
      RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

  std::transform(back_buffer.begin(), back_buffer.end(),
                 std::back_inserter(back_buffer_view), [&](auto &&img) {
                   return dev->create_image_view(
                       *img, swap_chain_format, 0, 1, 0, 1,
                       irr::video::E_TEXTURE_TYPE::ETT_2D);
                 });

  createDescriptorSets();
  fill_descriptor_set();

  fbo_pass1[0] = dev->create_frame_buffer(
      std::vector<const image_view_t *>{
          diffuse_color_view.get(), normal_view.get(),
          roughness_metalness_view.get(), back_buffer_view[0].get()},
      *depth_view, width, height, object_sunlight_pass.get());
  fbo_pass1[1] = dev->create_frame_buffer(
      std::vector<const image_view_t *>{
          diffuse_color_view.get(), normal_view.get(),
          roughness_metalness_view.get(), back_buffer_view[1].get()},
      *depth_view, width, height, object_sunlight_pass.get());

  fbo_pass2[0] = dev->create_frame_buffer(
      std::vector<const image_view_t *>{back_buffer_view[0].get()}, *depth_view,
      width, height, ibl_skyboss_pass.get());
  fbo_pass2[1] = dev->create_frame_buffer(
      std::vector<const image_view_t *>{back_buffer_view[1].get()}, *depth_view,
      width, height, ibl_skyboss_pass.get());

  Assimp::Importer importer;
  auto model = importer.ReadFile(std::string(SAMPLE_PATH) + "xue.b3d", 0);

  scene = std::make_unique<irr::scene::Scene>();
  xue = scene->addMeshSceneNode(std::make_unique<irr::scene::IMeshSceneNode>(
                                    *dev, model, *command_list,
                                    *cbv_srv_descriptors_heap, object_set.get(),
                                    model_set.get(), nullptr),
                                nullptr);

  big_triangle = dev->create_buffer(
      4 * 3 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE,
      usage_vertex);
  const auto &fullscreen_tri = std::array<float, 12>{
      -1.f, -3.f, 0.f, 2.f, 3.f, 1.f, 2.f, 0.f, -1.f, 1.f, 0.f, 0.f};

  memcpy(big_triangle->map_buffer(), fullscreen_tri.data(),
         fullscreen_tri.size() * sizeof(float));
  big_triangle->unmap_buffer();
  big_triangle_info = {{*big_triangle, 0,
                        4 * static_cast<uint32_t>(sizeof(float)),
                        4 * 3 * static_cast<uint32_t>(sizeof(float))}};

  command_list->make_command_list_executable();
  cmdqueue->submit_executable_command_list(*command_list, nullptr);
  cmdqueue->wait_for_command_queue_idle();
  // ibl
  ibl_utility ibl_util(*dev);
  command_list->start_command_list_recording(*command_allocator);
  sh_coefficients = ibl_util.computeSphericalHarmonics(*dev, *command_list,
                                                       *skybox_view, 1024);
  specular_cube =
      ibl_util.generateSpecularCubemap(*dev, *command_list, *skybox_view);
  std::tie(dfg_lut, dfg_lut_view) =
      ibl_util.getDFGLUT(*dev, *command_list, 128);

  specular_cube_view =
      dev->create_image_view(*specular_cube, irr::video::ECF_R16G16B16A16F, 0,
                             8, 0, 6, irr::video::E_TEXTURE_TYPE::ETT_CUBE);

  dev->set_image_view(*ibl_descriptor, 1, 11, *specular_cube_view);
  dev->set_image_view(*ibl_descriptor, 2, 12, *dfg_lut_view);
  dev->set_constant_buffer_view(*ibl_descriptor, 0, 10, *sh_coefficients,
                                27 * sizeof(float));
  ssao_util =
      std::make_unique<ssao_utility>(*dev, depth_buffer.get(), width, height);

  ssao_view = dev->create_image_view(*ssao_util->ssao_bilinear_result,
                                     irr::video::ECOLOR_FORMAT::ECF_R16F, 0, 1,
                                     0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
  dev->set_image_view(*rtt_descriptors, 4, 15, *ssao_view);

  command_list->set_pipeline_barrier(
      *ssao_util->linear_depth_buffer, RESOURCE_USAGE::undefined,
      RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
  command_list->set_pipeline_barrier(
      *ssao_util->ssao_result, RESOURCE_USAGE::undefined,
      RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
  command_list->set_pipeline_barrier(
      *ssao_util->gaussian_blurring_buffer, RESOURCE_USAGE::undefined,
      RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
  command_list->set_pipeline_barrier(
      *ssao_util->ssao_bilinear_result, RESOURCE_USAGE::undefined,
      RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
  command_list->make_command_list_executable();
  cmdqueue->submit_executable_command_list(*command_list, nullptr);
  cmdqueue->wait_for_command_queue_idle();

  fill_draw_commands();
  present_semaphore = dev->create_semaphore();
}

void MeshSample::createDescriptorSets() {
  ibl_descriptor =
      cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(
          10, {ibl_set.get()}, 3);
  scene_descriptor =
      cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(
          0, {scene_set.get()}, 3);
  input_attachments_descriptors =
      cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(
          5, {input_attachments_set.get()}, 4);
  rtt_descriptors =
      cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(
          5, {rtt_set.get()}, 5);
  sampler_descriptors = sampler_heap->allocate_descriptor_set_from_sampler_heap(
      0, {sampler_set.get()}, 2);
}

void MeshSample::createTextures() {
  clear_value_t clear_val = get_clear_value(irr::video::D24U8, 1., 0);
  depth_buffer = dev->create_image(
      irr::video::D24U8, width, height, 1, 1,
      usage_depth_stencil | usage_sampled | usage_input_attachment, &clear_val);
  clear_val = get_clear_value(irr::video::ECF_R8G8B8A8_UNORM, {0., 0., 0., 0.});
  diffuse_color = dev->create_image(
      irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, 1,
      usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
  roughness_metalness = dev->create_image(
      irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, 1,
      usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
  clear_val = get_clear_value(irr::video::ECF_R16G16F, {0., 0., 0., 0.});
  normal = dev->create_image(
      irr::video::ECF_R16G16F, width, height, 1, 1,
      usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
}

void MeshSample::fill_descriptor_set() {
  diffuse_color_view =
      dev->create_image_view(*diffuse_color, irr::video::ECF_R8G8B8A8_UNORM, 0,
                             1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
  normal_view =
      dev->create_image_view(*normal, irr::video::ECF_R16G16F, 0, 1, 0, 1,
                             irr::video::E_TEXTURE_TYPE::ETT_2D);
  roughness_metalness_view = dev->create_image_view(
      *roughness_metalness, irr::video::ECF_R8G8B8A8_UNORM, 0, 1, 0, 1,
      irr::video::E_TEXTURE_TYPE::ETT_2D);
  skybox_view =
      dev->create_image_view(*skybox_texture, irr::video::ECF_BC1_UNORM_SRGB, 0,
                             11, 0, 6, irr::video::E_TEXTURE_TYPE::ETT_CUBE);
  depth_view = dev->create_image_view(*depth_buffer, irr::video::D24U8, 0, 1, 0,
                                      1, irr::video::E_TEXTURE_TYPE::ETT_2D,
                                      irr::video::E_ASPECT::EA_DEPTH);

  // scene
  dev->set_image_view(*scene_descriptor, 2, 9, *skybox_view);
  dev->set_constant_buffer_view(*scene_descriptor, 0, 7, *scene_matrix,
                                sizeof(SceneData));
  dev->set_constant_buffer_view(*scene_descriptor, 1, 8, *sun_data,
                                sizeof(7 * sizeof(float)));

  // rtt
  dev->set_input_attachment(*input_attachments_descriptors, 0, 4,
                            *diffuse_color_view);
  dev->set_input_attachment(*input_attachments_descriptors, 1, 5, *normal_view);
  dev->set_input_attachment(*input_attachments_descriptors, 2, 14,
                            *roughness_metalness_view);
  dev->set_input_attachment(*input_attachments_descriptors, 3, 6, *depth_view);

  dev->set_image_view(*rtt_descriptors, 0, 4, *diffuse_color_view);
  dev->set_image_view(*rtt_descriptors, 1, 5, *normal_view);
  dev->set_image_view(*rtt_descriptors, 2, 14, *roughness_metalness_view);
  dev->set_image_view(*rtt_descriptors, 3, 6, *depth_view);

  bilinear_clamped_sampler =
      dev->create_sampler(SAMPLER_TYPE::BILINEAR_CLAMPED);
  anisotropic_sampler = dev->create_sampler(SAMPLER_TYPE::ANISOTROPIC);

  dev->set_sampler(*sampler_descriptors, 0, 3, *anisotropic_sampler);
  dev->set_sampler(*sampler_descriptors, 1, 13, *bilinear_clamped_sampler);
}

void MeshSample::load_program_and_pipeline_layout() {
  sampler_set = dev->get_object_descriptor_set(sampler_descriptor_set_type);
  object_set = dev->get_object_descriptor_set(object_descriptor_set_type);
  scene_set = dev->get_object_descriptor_set(scene_descriptor_set_type);
  input_attachments_set =
      dev->get_object_descriptor_set(input_attachments_descriptor_set_type);
  rtt_set = dev->get_object_descriptor_set(rtt_descriptor_set_type);
  model_set = dev->get_object_descriptor_set(model_descriptor_set_type);
  ibl_set = dev->get_object_descriptor_set(ibl_descriptor_set_type);

  object_sig =
      dev->create_pipeline_layout(std::vector<const descriptor_set_layout *>{
          model_set.get(), object_set.get(), scene_set.get(),
          sampler_set.get()});
  sunlight_sig =
      dev->create_pipeline_layout(std::vector<const descriptor_set_layout *>{
          input_attachments_set.get(), scene_set.get()});
  skybox_sig =
      dev->create_pipeline_layout(std::vector<const descriptor_set_layout *>{
          scene_set.get(), sampler_set.get()});
  ibl_sig =
      dev->create_pipeline_layout(std::vector<const descriptor_set_layout *>{
          rtt_set.get(), scene_set.get(), ibl_set.get(), sampler_set.get()});

  objectpso = get_skinned_object_pipeline_state(*dev, *object_sig,
                                                *object_sunlight_pass);
  sunlightpso =
      get_sunlight_pipeline_state(*dev, *sunlight_sig, *object_sunlight_pass);
  skybox_pso = get_skybox_pipeline_state(*dev, *skybox_sig, *ibl_skyboss_pass);
  ibl_pso = get_ibl_pipeline_state(*dev, *ibl_sig, *ibl_skyboss_pass);
}

DEFINE_bool(uses_debug_marker, false, "Uses debug marker (renderdoc only).");
DEFINE_bool(uses_debug_layer, false, "Uses debug layer.");

MeshSample::MeshSample() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(1280, 1024, "Window Title", nullptr, nullptr);

  std::tie(dev, chain, cmdqueue, width, height, swap_chain_format) =
      create_device_swapchain_and_graphic_presentable_queue(
          window, FLAGS_uses_debug_marker, FLAGS_uses_debug_layer);
  Init();
}

void MeshSample::fill_draw_commands() {
  for (unsigned i = 0; i < 2; i++) {
    command_list_for_back_buffer.push_back(
        command_allocator->create_command_list());
    command_list_t *current_cmd_list =
        command_list_for_back_buffer.back().get();
    current_cmd_list->start_command_list_recording(*command_allocator);
    //		current_cmd_list->set_pipeline_barrier(*back_buffer[i],
    // RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::RENDER_TARGET, 0,
    // irr::video::E_ASPECT::EA_COLOR);

    const auto &clearColor = std::array<float, 4>{.25f, .25f, 0.35f, 1.0f};
    current_cmd_list->begin_renderpass(
        *object_sunlight_pass, *fbo_pass1[i],
        std::vector<clear_value_t>{
            std::array<float, 4>{}, std::array<float, 4>{},
            std::array<float, 4>{}, std::array<float, 4>{},
            std::make_tuple(1.f, 0)},
        width, height);
#ifdef D3D12
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtt_to_use = {
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
            .Offset(0,
                    dev->object->GetDescriptorHandleIncrementSize(
                        D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
            .Offset(1,
                    dev->object->GetDescriptorHandleIncrementSize(
                        D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
            .Offset(2,
                    dev->object->GetDescriptorHandleIncrementSize(
                        D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
    };
    current_cmd_list->object->OMSetRenderTargets(
        rtt_to_use.size(), rtt_to_use.data(), false,
        &(fbo_pass1[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));
    clear_color(*current_cmd_list, fbo_pass1[i], clearColor);
    clear_depth_stencil(*current_cmd_list, fbo_pass1[i], 1., 0);

    current_cmd_list->object->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif
    current_cmd_list->set_graphic_pipeline_layout(*object_sig);
    current_cmd_list->set_descriptor_storage_referenced(
        *cbv_srv_descriptors_heap, sampler_heap.get());
    current_cmd_list->set_graphic_pipeline(*objectpso);
    current_cmd_list->bind_graphic_descriptor(2, *scene_descriptor,
                                              *object_sig);
    current_cmd_list->bind_graphic_descriptor(3, *sampler_descriptors,
                                              *object_sig);
    current_cmd_list->set_viewport(0.f, static_cast<float>(width), 0.f,
                                   static_cast<float>(height), 0.f, 1.f);
    current_cmd_list->set_scissor(0, width, 0, height);

    scene->fill_gbuffer_filling_command(*current_cmd_list, *object_sig);
#ifdef D3D12
    set_pipeline_barrier(
        *current_cmd_list, *diffuse_color, RESOURCE_USAGE::RENDER_TARGET,
        RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(
        *current_cmd_list, *normal, RESOURCE_USAGE::RENDER_TARGET,
        RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(
        *current_cmd_list, *roughness_metalness, RESOURCE_USAGE::RENDER_TARGET,
        RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(
        *current_cmd_list, *depth_buffer, RESOURCE_USAGE::DEPTH_WRITE,
        RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_DEPTH);
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> present_rtt = {
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
            .Offset(3,
                    dev->object->GetDescriptorHandleIncrementSize(
                        D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
    };
    current_cmd_list->object->OMSetRenderTargets(
        present_rtt.size(), present_rtt.data(), false, nullptr);
#endif
    current_cmd_list->next_subpass();
    current_cmd_list->set_graphic_pipeline_layout(*sunlight_sig);
    current_cmd_list->set_descriptor_storage_referenced(
        *cbv_srv_descriptors_heap, sampler_heap.get());
    current_cmd_list->bind_graphic_descriptor(0, *input_attachments_descriptors,
                                              *sunlight_sig);
    current_cmd_list->bind_graphic_descriptor(1, *scene_descriptor,
                                              *sunlight_sig);
    current_cmd_list->set_graphic_pipeline(*sunlightpso);
    current_cmd_list->bind_vertex_buffers(0, big_triangle_info);
    current_cmd_list->draw_non_indexed(3, 1, 0, 0);
    current_cmd_list->end_renderpass();
    ssao_util->fill_command_list(*dev, *current_cmd_list, *depth_buffer, 1.f,
                                 100.f, big_triangle_info);
#ifdef D3D12
    current_cmd_list->object->OMSetRenderTargets(
        present_rtt.size(), present_rtt.data(), false, nullptr);
#endif // !D3D12
    current_cmd_list->begin_renderpass(*ibl_skyboss_pass, *fbo_pass2[i],
                                       std::vector<clear_value_t>{}, width,
                                       height);
    current_cmd_list->set_graphic_pipeline_layout(*ibl_sig);
    current_cmd_list->set_descriptor_storage_referenced(
        *cbv_srv_descriptors_heap, sampler_heap.get());
    current_cmd_list->bind_graphic_descriptor(0, *rtt_descriptors, *ibl_sig);
    current_cmd_list->bind_graphic_descriptor(1, *scene_descriptor, *ibl_sig);
    current_cmd_list->bind_graphic_descriptor(2, *ibl_descriptor, *ibl_sig);
    current_cmd_list->bind_graphic_descriptor(3, *sampler_descriptors,
                                              *ibl_sig);
    current_cmd_list->set_graphic_pipeline(*ibl_pso);
    current_cmd_list->bind_vertex_buffers(0, big_triangle_info);
    current_cmd_list->draw_non_indexed(3, 1, 0, 0);
#ifdef D3D12
    current_cmd_list->object->OMSetRenderTargets(
        present_rtt.size(), present_rtt.data(), false,
        &(fbo_pass1[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));
    set_pipeline_barrier(
        *current_cmd_list, *depth_buffer, RESOURCE_USAGE::READ_GENERIC,
        RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH);
#endif
    current_cmd_list->next_subpass();
    current_cmd_list->set_graphic_pipeline_layout(*skybox_sig);
    current_cmd_list->bind_graphic_descriptor(0, *scene_descriptor,
                                              *skybox_sig);
    current_cmd_list->bind_graphic_descriptor(1, *sampler_descriptors,
                                              *skybox_sig);
    current_cmd_list->set_graphic_pipeline(*skybox_pso);
    current_cmd_list->bind_vertex_buffers(0, big_triangle_info);
    current_cmd_list->draw_non_indexed(3, 1, 0, 0);
    current_cmd_list->end_renderpass();
#ifdef D3D12
    set_pipeline_barrier(
        *current_cmd_list, *diffuse_color, RESOURCE_USAGE::READ_GENERIC,
        RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(
        *current_cmd_list, *normal, RESOURCE_USAGE::READ_GENERIC,
        RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(
        *current_cmd_list, *roughness_metalness, RESOURCE_USAGE::READ_GENERIC,
        RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
#endif // !D3D12
       //		current_cmd_list->set_pipeline_barrier(*back_buffer[i],
       // RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::PRESENT, 0,
    // irr::video::E_ASPECT::EA_COLOR);
    current_cmd_list->make_command_list_executable();
  }
}

void MeshSample::Draw() {
  scene->update(*dev);

  auto &&tmp = *static_cast<SceneData *>(scene_matrix->map_buffer());
  const float horizon_angle_in_radian = horizon_angle * 3.14f / 100.f;
  auto View = glm::lookAtLH(
      glm::vec3(0., 2. * sin(horizon_angle_in_radian),
                -2. * cos(horizon_angle_in_radian)),
      glm::vec3(0., cos(horizon_angle_in_radian), sin(horizon_angle_in_radian)),
      glm::vec3(0., 1., 0.));
  tmp.ViewMatrix = View;
  tmp.InverseViewMatrix = glm::inverse(View);
  const auto &Perspective =
      glm::perspective(70.f / 180.f * 3.14f, 1.f, 1.f, 1000.f);
  tmp.ProjectionMatrix = Perspective;
  tmp.InverseProjectionMatrix = glm::inverse(Perspective);
  scene_matrix->unmap_buffer();

  const auto &sun_tmp =
      gsl::span<float, 7>(static_cast<float *>(sun_data->map_buffer()), 7);
  sun_tmp[0] = 0.;
  sun_tmp[1] = 1.;
  sun_tmp[2] = 0.;
  sun_tmp[3] = .5f;
  sun_tmp[4] = 10.;
  sun_tmp[5] = 10.;
  sun_tmp[6] = 10.;
  sun_data->unmap_buffer();

  //	double intpart;
  //	float frame = (float)modf(timer / 10000., &intpart);
  //	frame *= 300.f;
  /*        loader->AnimatedMesh.animateMesh(frame, 1.f);
                  loader->AnimatedMesh.skinMesh(1.f);

                  memcpy(map_buffer(dev, jointbuffer),
     loader->AnimatedMesh.JointMatrixes.data(),
     loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));*/
  // unmap_buffer(dev, jointbuffer);

  const auto &current_backbuffer =
      chain->get_next_backbuffer_id(*present_semaphore);
  cmdqueue->submit_executable_command_list(
      *command_list_for_back_buffer[current_backbuffer],
      present_semaphore.get());
  cmdqueue->wait_for_command_queue_idle();
  chain->present(*cmdqueue, current_backbuffer);
}

DEFINE_string(backend, "vulkan", "renderer to use (vulkan or dx12)");

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  /*	if (FLAGS_backend == "vulkan")
                  return 0;
          else if (FLAGS_backend == "dx12")
                  return 0;*/

  MeshSample sample;
  sample.Loop();
}
