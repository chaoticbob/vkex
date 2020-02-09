/*
 Copyright 2018-2019 Google Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "common/AssetUtil.h"
#include "shaders/Common.h"
#include "common/DebugUi.h"
#include "common/SimpleRenderPass.h"
#include "vkex/Application.h"

#if defined(VKEX_GGP)
const uint32_t k_window_width  = 1920;
const uint32_t k_window_height = 1080;
#else
const uint32_t k_window_width  = 1280;
const uint32_t k_window_height = 720;
#endif

using float3   = vkex::float3;
using float3x3 = vkex::float3x3;
using float4x4 = vkex::float4x4;

using ViewConstants = vkex::ConstantBufferData<vkex::ViewConstantsData>;

VkFormat k_color_format         = VK_FORMAT_R8G8B8A8_UNORM;
VkFormat k_depth_stencil_format = VK_FORMAT_D32_SFLOAT;

struct PerFrameData
{
  vkex::CommandPool   command_pool            = nullptr;
  vkex::CommandBuffer command_buffer          = nullptr;
  vkex::Semaphore     work_complete_semaphore = nullptr;
  vkex::Fence         work_complete_fence     = nullptr;
  SimpleRenderPass    draw_render_pass        = {};
  vkex::DescriptorSet descriptor_set          = nullptr;
  vkex::Buffer        constant_buffer         = nullptr;
};

class VkexInfoApp : public vkex::Application
{
public:
  VkexInfoApp()
    : vkex::Application(k_window_width, k_window_height, "03_render_work")
  {
  }
  virtual ~VkexInfoApp()
  {
  }

  void Configure(const vkex::ArgParser& args, vkex::Configuration& configuration);
  void Setup();
  void Update(double frame_elapsed_time)
  {
  }
  void Render(Application::RenderData* p_current_render_data, Application::PresentData* p_current_present_data);
  void Present(vkex::Application::PresentData* p_current_present_data);

  void SetupPerFrameObjects();

private:
  std::vector<PerFrameData> m_per_frame_data        = {};
  vkex::ShaderProgram       m_color_shader          = nullptr;
  vkex::DescriptorSetLayout m_descriptor_set_layout = nullptr;
  vkex::DescriptorPool      m_descriptor_pool       = nullptr;
  vkex::PipelineLayout      m_color_pipeline_layout = nullptr;
  vkex::GraphicsPipeline    m_color_pipeline        = nullptr;
  ViewConstants             m_view_constants        = {};
  vkex::Buffer              m_vertex_buffer         = nullptr;
  vkex::Texture             m_texture               = nullptr;
  vkex::Sampler             m_sampler               = nullptr;
};

void VkexInfoApp::Configure(const vkex::ArgParser& args, vkex::Configuration& configuration)
{
  // Force present mode to VK_PRESENT_MODE_MAILBOX_KHR for now...because #reasons
  configuration.window.resizeable                       = false;
  configuration.swapchain.paced_frame_rate              = 60;
  configuration.swapchain.present_mode                  = VK_PRESENT_MODE_IMMEDIATE_KHR;
  configuration.swapchain.depth_stencil_format          = VK_FORMAT_D32_SFLOAT;
  configuration.swapchain.color_load_op                 = VK_ATTACHMENT_LOAD_OP_LOAD;
  configuration.graphics_debug.enable                   = true;
  configuration.graphics_debug.message_severity.info    = false;
  configuration.graphics_debug.message_severity.warning = true;
  configuration.graphics_debug.message_severity.error   = true;
  configuration.graphics_debug.message_type.validation  = true;
  configuration.enable_imgui                            = true;
}

void VkexInfoApp::SetupPerFrameObjects()
{
  const uint32_t frame_count = GetConfiguration().frame_count;
  m_per_frame_data.resize(frame_count);

  for (uint32_t frame_index = 0; frame_index < frame_count; ++frame_index) {
    PerFrameData& per_frame_data = m_per_frame_data[frame_index];

    // Command pool
    {
      vkex::CommandPoolCreateInfo create_info     = {};
      create_info.flags.bits.reset_command_buffer = true;
      VKEX_CALL(GetDevice()->CreateCommandPool(create_info, &per_frame_data.command_pool));
    }

    // Command buffer
    {
      vkex::CommandBufferAllocateInfo allocate_info = {};
      allocate_info.command_buffer_count            = 1;
      VKEX_CALL(per_frame_data.command_pool->AllocateCommandBuffer(allocate_info, &per_frame_data.command_buffer));
    }

    // Work complete semaphore
    {
      vkex::SemaphoreCreateInfo create_info = {};
      VKEX_CALL(GetDevice()->CreateSemaphore(create_info, &per_frame_data.work_complete_semaphore));
    }

    // Work complete fence
    {
      vkex::FenceCreateInfo create_info = {};
      create_info.flags.bits.signaled   = true;
      VKEX_CALL(GetDevice()->CreateFence(create_info, &per_frame_data.work_complete_fence));
    }

    // Main render pass
    {
      uint32_t          width                = k_window_width;
      uint32_t          height               = k_window_height;
      VkFormat          color_format         = k_color_format;
      VkFormat          depth_stencil_format = k_depth_stencil_format;
      SimpleRenderPass& render_pass          = per_frame_data.draw_render_pass;
      VKEX_CALL(CreateSimpleRenderPass(GetDevice(), GetGraphicsQueue(), width, height, color_format, depth_stencil_format, &render_pass));
    }

    // Descriptor sets
    {
      vkex::DescriptorSetAllocateInfo allocate_info = {};
      allocate_info.layouts.push_back(m_descriptor_set_layout);
      VKEX_CALL(m_descriptor_pool->AllocateDescriptorSets(allocate_info, &per_frame_data.descriptor_set));
    }

    // Constant buffer
    {
      vkex::BufferCreateInfo create_info = {};
      create_info.size                   = m_view_constants.size;
      create_info.committed              = true;
      create_info.memory_usage           = VMA_MEMORY_USAGE_CPU_TO_GPU;
      VKEX_CALL(GetDevice()->CreateConstantBuffer(create_info, &per_frame_data.constant_buffer));
    }

    // Update descriptors
    {
      per_frame_data.descriptor_set->UpdateDescriptor(VKEX_SHADER_CONSTANTS_BASE_REGISTER, per_frame_data.constant_buffer);
      per_frame_data.descriptor_set->UpdateDescriptor(VKEX_SHADER_TEXTURE_BASE_REGISTER, m_texture);
      per_frame_data.descriptor_set->UpdateDescriptor(VKEX_SHADER_SAMPLER_BASE_REGISTER, m_sampler);
    }
  }
}

void VkexInfoApp::Setup()
{
  // Geometry data
  vkex::PlatonicSolid::Options cube_options         = {};
  cube_options.tex_coords                           = true;
  cube_options.normals                              = true;
  vkex::PlatonicSolid           cube                = vkex::PlatonicSolid::Cube(cube_options);
  const vkex::VertexBufferData* p_vertex_buffer_cpu = cube.GetVertexBufferByIndex(0);

  // Shader program
  {
    auto vs = asset_util::LoadFile(GetAssetPath("shaders/DiffuseTexture.vs.spv"));
    VKEX_ASSERT_MSG(!vs.empty(), "Vertex shader failed to load!");
    auto ps = asset_util::LoadFile(GetAssetPath("shaders/DiffuseTexture.ps.spv"));
    VKEX_ASSERT_MSG(!ps.empty(), "Pixel shader failed to load!");
    VKEX_CALL(vkex::CreateShaderProgram(GetDevice(), vs, ps, &m_color_shader));
  }

  // Descriptor set layouts
  {
    const vkex::ShaderInterface&        shader_interface = m_color_shader->GetInterface();
    vkex::DescriptorSetLayoutCreateInfo create_info      = ToVkexCreateInfo(shader_interface.GetSet(0));
    VKEX_CALL(GetDevice()->CreateDescriptorSetLayout(create_info, &m_descriptor_set_layout));
  }

  // Descriptor pool
  {
    uint32_t frame_count = GetConfiguration().frame_count;

    const vkex::ShaderInterface&   shader_interface = m_color_shader->GetInterface();
    vkex::DescriptorPoolCreateInfo create_info      = {};
    create_info.pool_sizes                          = frame_count * shader_interface.GetDescriptorPoolSizes();
    VKEX_CALL(GetDevice()->CreateDescriptorPool(create_info, &m_descriptor_pool));
  }

  // Pipeline layout
  {
    vkex::PipelineLayoutCreateInfo create_info = {};
    create_info.descriptor_set_layouts.push_back(vkex::ToVulkan(m_descriptor_set_layout));
    vkex::Result vkex_result = vkex::Result::Undefined;
    VKEX_CALL(GetDevice()->CreatePipelineLayout(create_info, &m_color_pipeline_layout));
  }

  // Pipeline
  {
    vkex::VertexBindingDescription vertex_binding_descriptions =
      p_vertex_buffer_cpu->GetVertexBindingDescription();

    vkex::GraphicsPipelineCreateInfo create_info = {};
    create_info.shader_program                   = m_color_shader;
    create_info.vertex_binding_descriptions      = {vertex_binding_descriptions};
    create_info.samples                          = VK_SAMPLE_COUNT_1_BIT;
    create_info.depth_test_enable                = true;
    create_info.depth_write_enable               = true;
    create_info.pipeline_layout                  = m_color_pipeline_layout;
    create_info.rtv_formats                      = {k_color_format};
    create_info.dsv_format                       = k_depth_stencil_format;
    vkex::Result vkex_result                     = vkex::Result::Undefined;
    VKEX_CALL(GetDevice()->CreateGraphicsPipeline(create_info, &m_color_pipeline));
  }

  // Vertex buffer
  {
    size_t                 size        = p_vertex_buffer_cpu->GetDataSize();
    vkex::BufferCreateInfo create_info = {};
    create_info.size                   = size;
    create_info.committed              = true;
    create_info.memory_usage           = VMA_MEMORY_USAGE_CPU_TO_GPU;
    VKEX_CALL(GetDevice()->CreateVertexBuffer(create_info, &m_vertex_buffer));
    VKEX_CALL(
      m_vertex_buffer->Copy(p_vertex_buffer_cpu->GetDataSize(), p_vertex_buffer_cpu->GetData()));
  }

  // Texture
  {
    const bool host_visible = false;

    // Load file data
    auto image_file_path = GetAssetPath("textures/box_panel.jpg");
    VKEX_CALL(asset_util::CreateTexture(
      image_file_path,
      GetGraphicsQueue(),
      host_visible,
      &m_texture));
  }

  // Sampler
  {
    vkex::SamplerCreateInfo create_info = {};
    create_info.min_filter              = VK_FILTER_LINEAR;
    create_info.mag_filter              = VK_FILTER_LINEAR;
    create_info.mipmap_mode             = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.min_lod                 = 0.0f;
    create_info.max_lod                 = 15.0f;
    VKEX_CALL(GetDevice()->CreateSampler(create_info, &m_sampler));
  }

  // Setup per frame objects
  SetupPerFrameObjects();
}

void VkexInfoApp::Render(Application::RenderData* p_current_render_data, Application::PresentData* p_current_present_data)
{
  const uint32_t frame_index         = GetCurrentFrameIndex();
  PerFrameData&  per_frame_data      = m_per_frame_data[frame_index];
  vkex::Fence&   work_complete_fence = per_frame_data.work_complete_fence;

  VkResult vk_result = work_complete_fence->WaitForAndResetFence();
  VKEX_ASSERT(vk_result == VK_SUCCESS);

  // Update constant buffer
  {
    float3            eye    = float3(0, 1, 2);
    float3            center = float3(0, 0, 0);
    float3            up     = float3(0, 1, 0);
    float             aspect = GetWindowAspect();
    vkex::PerspCamera camera(eye, center, up, 60.0f, aspect);

    float    t = GetFrameStartTime();
    float4x4 M = glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t / 2.0f, float3(0, 0, 1));
    float4x4 V = camera.GetViewMatrix();
    float4x4 P = camera.GetProjectionMatrix();

    m_view_constants.data.M   = M;
    m_view_constants.data.V   = V;
    m_view_constants.data.P   = P;
    m_view_constants.data.MVP = P * V * M;
    m_view_constants.data.N   = glm::inverseTranspose(float3x3(M));
    m_view_constants.data.LP  = float3(0, 3, 5);

    VKEX_CALL(per_frame_data.constant_buffer->Copy(m_view_constants.size, &m_view_constants.data));
  }

  // Build render work command buffer
  vkex::CommandBuffer& cmd = per_frame_data.command_buffer;
  cmd->Begin();
  {
    auto& render_pass    = per_frame_data.draw_render_pass.render_pass;
    auto& descriptor_set = per_frame_data.descriptor_set;

    VkClearValue              rtv_clear    = vkex::ClearColorValue(0.23f, 0.23f, 0.23f, 1);
    VkClearValue              dsv_clear    = vkex::ClearDepthStencilValue();
    std::vector<VkClearValue> clear_values = {rtv_clear, dsv_clear};

    // Draw a cube to "draw" render pass
    cmd->CmdBeginRenderPass(per_frame_data.draw_render_pass.render_pass, &clear_values);
    {
      cmd->CmdSetViewport(render_pass->GetFullRenderArea());
      cmd->CmdSetScissor(render_pass->GetFullRenderArea());
      cmd->CmdBindPipeline(m_color_pipeline);
      cmd->CmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_color_pipeline_layout, 0, {*descriptor_set});
      cmd->CmdBindVertexBuffers(m_vertex_buffer);
      cmd->CmdDraw(36, 1, 0, 0);
    }
    cmd->CmdEndRenderPass();
  }
  cmd->End();

  // Submit render work
  {
    vkex::SubmitInfo submit_info = {};

    if (p_current_present_data->GetPrevious() != nullptr) {
      const Application::PresentData* p_previous = p_current_present_data->GetPrevious();
      submit_info.AddWaitSemaphore(p_previous->GetWorkCompleteForRenderSemaphore());
    }

    submit_info.AddCommandBuffer(cmd);
    submit_info.AddSignalSemaphore(per_frame_data.work_complete_semaphore);
    submit_info.SetFence(work_complete_fence);

    VKEX_CALL(GetGraphicsQueue()->Submit(submit_info));
  }

  // Add render work's signal semaphore to be waited on by present work
  p_current_render_data->ClearWaitSemaphores();
  p_current_render_data->AddWaitSemaphore(per_frame_data.work_complete_semaphore);
}

void VkexInfoApp::Present(vkex::Application::PresentData* p_data)
{
  auto cmd         = p_data->GetCommandBuffer();
  auto render_pass = p_data->GetRenderPass();

  // Build present work command buffer
  cmd->Begin();
  {
    uint32_t      frame_index    = p_data->GetFrameIndex();
    PerFrameData& per_frame_data = m_per_frame_data[frame_index];

    // Blit image from "draw" render pass to swapchain image
    {
      cmd->CmdTransitionImageLayout(
        per_frame_data.draw_render_pass.rtv_texture,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

      cmd->CmdTransitionImageLayout(
        render_pass->GetRtvs()[0]->GetColorImageView()->GetImage(),
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

      cmd->CmdBlitImage(
        per_frame_data.draw_render_pass.rtv_texture->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        per_frame_data.draw_render_pass.render_pass->GetFullRenderArea(),
        render_pass->GetRtvs()[0]->GetColorImageView()->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        render_pass->GetFullRenderArea());

      cmd->CmdTransitionImageLayout(
        per_frame_data.draw_render_pass.rtv_texture,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

      cmd->CmdTransitionImageLayout(
        render_pass->GetRtvs()[0]->GetColorImageView()->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_PRESENT_BIT);
    }

    // Draw IMGUI
    VkClearValue              rtv_clear    = vkex::ClearColorValue(0, 0, 0, 0);
    VkClearValue              dsv_clear    = vkex::ClearDepthStencilValue();
    std::vector<VkClearValue> clear_values = {rtv_clear, dsv_clear};
    cmd->CmdBeginRenderPass(render_pass, &clear_values);
    {
      this->DrawDebugApplicationInfo();
      this->DrawImGui(cmd);
    }
    cmd->CmdEndRenderPass();
  }
  cmd->End();
}

int main(int argn, char** argv)
{
  VkexInfoApp  app;
  vkex::Result vkex_result = app.Run(argn, argv);
  if (!vkex_result) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
