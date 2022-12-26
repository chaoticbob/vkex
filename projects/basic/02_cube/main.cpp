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
#include "vkex/Application.h"

const uint32_t k_window_width  = 1280;
const uint32_t k_window_height = 720;

using float3   = vkex::float3;
using float3x3 = vkex::float3x3;
using float4x4 = vkex::float4x4;

using ViewConstants = vkex::ConstantBufferData<vkex::ViewConstantsData>;

struct PerFrameData
{
    vkex::DescriptorSet descriptor_set  = nullptr;
    vkex::Buffer        constant_buffer = nullptr;
};

class VkexInfoApp : public vkex::Application
{
public:
    VkexInfoApp()
        : vkex::Application(k_window_width, k_window_height, "02_cube")
    {
    }
    virtual ~VkexInfoApp()
    {
    }

    void Configure(const vkex::ArgParser& args, vkex::Configuration& configuration);
    void Setup();
    void Update(double frame_elapsed_time) {}
    void Render(vkex::RenderData* p_data);
    void Present(vkex::PresentData* p_data);

private:
    std::vector<PerFrameData> m_per_frame_data        = {};
    vkex::ShaderProgram       m_color_shader          = nullptr;
    vkex::DescriptorSetLayout m_descriptor_set_layout = nullptr;
    vkex::DescriptorPool      m_color_descriptor_pool = nullptr;
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
    configuration.swapchain.present_mode                  = VK_PRESENT_MODE_MAILBOX_KHR;
    configuration.swapchain.depth_stencil_format          = VK_FORMAT_D32_SFLOAT;
    configuration.graphics_debug.enable                   = true;
    configuration.graphics_debug.message_severity.info    = false;
    configuration.graphics_debug.message_severity.warning = true;
    configuration.graphics_debug.message_severity.error   = true;
    configuration.graphics_debug.message_type.validation  = true;
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
        VKEX_CALL(GetDevice()->CreateDescriptorPool(create_info, &m_color_descriptor_pool));
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
        create_info.color_formats                    = {GetConfiguration().swapchain.color_format};
        create_info.depth_stencil_format             = GetConfiguration().swapchain.depth_stencil_format;

        vkex::Result vkex_result = vkex::Result::Undefined;
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

    // Per frame data
    {
        const uint32_t frame_count = GetFrameCount();
        m_per_frame_data.resize(frame_count);

        for (uint32_t frame_index = 0; frame_index < frame_count; ++frame_index) {
            PerFrameData& per_frame_data = m_per_frame_data[frame_index];

            // Descriptor sets
            {
                vkex::DescriptorSetAllocateInfo allocate_info = {};
                allocate_info.layouts.push_back(m_descriptor_set_layout);
                VKEX_CALL(m_color_descriptor_pool->AllocateDescriptorSets(allocate_info, &per_frame_data.descriptor_set));
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
}

void VkexInfoApp::Render(vkex::RenderData* p_data)
{
}

void VkexInfoApp::Present(vkex::PresentData* p_present_data)
{
    // Frame data
    uint32_t      frame_index = p_present_data->GetFrameIndex();
    PerFrameData& frame_data  = m_per_frame_data[frame_index];

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

        VKEX_CALL(frame_data.constant_buffer->Copy(m_view_constants.size, &m_view_constants.data));
    }

    // Build command buffer
    auto cmd = p_present_data->GetCommandBuffer();
    cmd->Begin();
    {
        auto rendering_info = vkex::RenderingInfo::LoadOpClear(
            {p_present_data->GetColorAttachment()},
            p_present_data->GetDepthStencilAttachment());

        // Draw spinning cube
        cmd->CmdBeginRendering(rendering_info);
        {
            cmd->CmdSetViewport(rendering_info.render_area);
            cmd->CmdSetScissor(rendering_info.render_area);
            cmd->CmdBindPipeline(m_color_pipeline);
            cmd->CmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_color_pipeline_layout, 0, {*frame_data.descriptor_set});
            cmd->CmdBindVertexBuffers(m_vertex_buffer);
            cmd->CmdDraw(36, 1, 0, 0);

            // Application Info
            this->DrawDebugApplicationInfo();
            this->DrawImGui(cmd);
        }
        cmd->CmdEndRendering();
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