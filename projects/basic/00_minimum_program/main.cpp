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

#include "vkex/Application.h"
#include "shaders/Common.h"
#include "common/AssetUtil.h"
#include "common/DebugUi.h"

const uint32_t k_window_width  = 1280;
const uint32_t k_window_height = 720;

class VkexInfoApp : public vkex::Application
{
public:
    VkexInfoApp()
        : vkex::Application(k_window_width, k_window_height, "00_minimum_program") {}
    virtual ~VkexInfoApp() {}

    void Configure(const vkex::ArgParser& args, vkex::Configuration& configuration);
    void Present(vkex::PresentData* p_data);

private:
};

void VkexInfoApp::Configure(const vkex::ArgParser& args, vkex::Configuration& configuration)
{
    // Force present mode to VK_PRESENT_MODE_MAILBOX_KHR for now...because #reasons
    configuration.window.resizeable                       = true;
    configuration.swapchain.paced_frame_rate              = 60;
    configuration.swapchain.present_mode                  = VK_PRESENT_MODE_MAILBOX_KHR;
    configuration.swapchain.depth_stencil_format          = VK_FORMAT_D32_SFLOAT;
    configuration.graphics_debug.enable                   = true;
    configuration.graphics_debug.message_severity.info    = false;
    configuration.graphics_debug.message_severity.warning = true;
    configuration.graphics_debug.message_severity.error   = true;
    configuration.graphics_debug.message_type.validation  = true;
}

void VkexInfoApp::Present(vkex::PresentData* p_present_data)
{
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
            // Application Info - this isn't needed it's just here to draw something
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