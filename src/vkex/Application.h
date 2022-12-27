/*
 Copyright 2018-2023 Google Inc.

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

#ifndef __VKEX_APPLICATION_H__
#define __VKEX_APPLICATION_H__

#include "vkex/ArgParser.h"
#include "vkex/Bitmap.h"
#include "vkex/Camera.h"
#include "vkex/Cast.h"
#include "vkex/FileSystem.h"
#include "vkex/Geometry.h"
#include "vkex/Instance.h"
#include "vkex/Timer.h"
#include "vkex/ToString.h"
#include "vkex/Transform.h"

#include <GLFW/glfw3.h>
#if defined(VKEX_LINUX)
#    define GLFW_EXPOSE_NATIVE_X11
#elif defined(VKEX_WIN32)
#    define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

//
// On Linux, XCB has macro called 'Success' that causes
// compile issues since the compiler will attempt to
// expand it where vkex::Result::Success is used.
//
#if defined(VKEX_LINUX) && defined(Success)
#    undef Success
#endif

#include <imgui.h>

#include <mutex>
#include <thread>
#include <unordered_map>

namespace vkex {

/** @enum MouseButton
 *
 */
enum MouseButton
{
    Left   = 0x00000001,
    Right  = 0x00000002,
    Middle = 0x00000004,
};

/* @enum CursorMode
 *
 */
enum CursorMode
{
    Visible = 0,
    Hidden,
    Captured,
};

/** @enum Keyboard
 *
 */
enum KeyboardInput
{
    kKeySpace,
    kKeyApostrophe,
    kKeyComma,
    kKeyMinus,
    kKeyPeriod,
    kKeySlash,
    kKey0,
    kKey1,
    kKey2,
    kKey3,
    kKey4,
    kKey5,
    kKey6,
    kKey7,
    kKey8,
    kKey9,
    kKeySemicolon,
    kKeyEqual,
    kKeyA,
    kKeyB,
    kKeyC,
    kKeyD,
    kKeyE,
    kKeyF,
    kKeyG,
    kKeyH,
    kKeyI,
    kKeyJ,
    kKeyK,
    kKeyL,
    kKeyM,
    kKeyN,
    kKeyO,
    kKeyP,
    kKeyQ,
    kKeyR,
    kKeyS,
    kKeyT,
    kKeyU,
    kKeyV,
    kKeyW,
    kKeyX,
    kKeyY,
    kKeyZ,
    kKeyLeftBracket,
    kKeyBackslash,
    kKeyRightBracket,
    kKeyGraveAccent,
    kKeyWorld_1,
    kKeyWorld_2,
    kKeyEscape,
    kKeyEnter,
    kKeyTab,
    kKeyBackspace,
    kKeyInsert,
    kKeyDelete,
    kKeyRight,
    kKeyLeft,
    kKeyDown,
    kKeyUp,
    kKeyPageUp,
    kKeyPageDown,
    kKeyHome,
    kKeyEnd,
    kKeyCapsLock,
    kKeyScrollLock,
    kKeyNumLock,
    kKeyPrintScreen,
    kKeyPause,
    kKeyF1,
    kKeyF2,
    kKeyF3,
    kKeyF4,
    kKeyF5,
    kKeyF6,
    kKeyF7,
    kKeyF8,
    kKeyF9,
    kKeyF10,
    kKeyF11,
    kKeyF12,
    kKeyF13,
    kKeyF14,
    kKeyF15,
    kKeyF16,
    kKeyF17,
    kKeyF18,
    kKeyF19,
    kKeyF20,
    kKeyF21,
    kKeyF22,
    kKeyF23,
    kKeyF24,
    kKeyF25,
    kKeyKeyPad0,
    kKeyKeyPad1,
    kKeyKeyPad2,
    kKeyKeyPad3,
    kKeyKeyPad4,
    kKeyKeyPad5,
    kKeyKeyPad6,
    kKeyKeyPad7,
    kKeyKeyPad8,
    kKeyKeyPad9,
    kKeyKeyPadDecimal,
    kKeyKeyPadDivide,
    kKeyKeyPadMultiply,
    kKeyKeyPadSubtract,
    kKeyKeyPadAdd,
    kKeyKeyPadEnter,
    kKeyKeyPadEqual,
    kKeyLeftShift,
    kKeyLeftControl,
    kKeyLeftAlt,
    kKeyLeftSuper,
    kKeyRightShift,
    kKeyRightControl,
    kKeyRightAlt,
    kKeyRightSuper,
    kKeyMenu,
    kNumKeys
};

/** @enum ApplicationMode
 *
 *
 */
enum ApplicationMode
{
    APPLICATION_MODE_WINDOW = 0,
    APPLICATION_MODE_HEADLESS
};

/** @enum Joystick
 *
 */
enum JoystickInput
{
};

// Forward declaration
class Application;

/** @class HistoryT
 *
 */
template <typename T, size_t SizeValue>
class HistoryT
{
public:
    HistoryT() {}
    ~HistoryT() {}

    const T& operator[](size_t n) const
    {
        return m_data[n];
    }

    const T* data() const
    {
        const T* ptr = m_data.data();
        return ptr;
    }

    size_t size() const
    {
        return m_data.size();
    }

    void push_back(const T& value)
    {
        if (m_data.size() == SizeValue) {
            for (size_t i = 0; i < (SizeValue - 1); ++i) {
                m_data[i] = m_data[i + 1];
            }
            m_data[SizeValue - 1] = value;
        }
        else {
            m_data.push_back(value);
        }
    }

private:
    std::vector<T> m_data;
};

/** @struct TimeRange
 *
 */
struct TimeRange
{
    float start;
    float end;
    float diff;
};

/** @struct Configuration
 *
 */
struct Configuration
{
    // Application mode
    //
    // Default: APPLICATION_MODE_WINDOW
    //
    ApplicationMode mode;

    // Application name
    std::string name;

    // Device criteria
    PhysicalDeviceCriteria device_criteria;

    // Number of in flight frames
    //
    // Default: 2
    //
    uint32_t frame_count;

    // Window
    //
    // Ignored if application 'mode' is APPLICATION_MODE_WINDOW.
    //
    struct
    {
        uint32_t   width;
        uint32_t   height;
        bool       resizeable;
        bool       fullscreen;
        bool       borderless;
        CursorMode cursor_mode;
    } window;

    // Swapchain
    //
    // Ignored if application 'mode' is APPLICATION_MODE_WINDOW.
    //
    struct
    {
        // Number of swapchain images
        //
        // Default: Number of in flight frames + 1
        // Bounds : [minImageCount, maxImageCount]
        //
        // minImageCount is VkSurfaceCapabilitiesKHR::minImageCount
        // maxImageCount is VkSurfaceCapabilitiesKHR::maxImageCount
        //
        uint32_t image_count;

        // Default: VK_FORMAT_B8G8R8A8_UNORM
        VkFormat color_format;

        // Default: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        VkColorSpaceKHR color_space;

        // Default: VK_FORMAT_UNDEFINED (if struct is initialized to zero)
        VkFormat depth_stencil_format;

        // Default: VK_PRESENT_MODE_IMMEDIATE_KHR
        VkPresentModeKHR present_mode;

        // Default: { 0.0f, 0.0f, 0.0f, 0.0f, }
        VkClearColorValue rtv_clear_value;

        // Default: { 1.0f, 0xFF }
        VkClearDepthStencilValue dsv_clear_value;

        // Default: VK_ATTACHMENT_LOAD_OP_CLEAR
        VkAttachmentLoadOp color_load_op;

        // Default: VK_ATTACHMENT_STORE_OP_STORE
        VkAttachmentStoreOp color_store_op;

        // Default: VK_ATTACHMENT_LOAD_OP_CLEAR
        VkAttachmentLoadOp depth_load_op;

        // Default: VK_ATTACHMENT_STORE_OP_STORE
        VkAttachmentStoreOp depth_store_op;

        // Default: VK_ATTACHMENT_LOAD_OP_CLEAR
        VkAttachmentLoadOp stencil_load_op;

        // Default: VK_ATTACHMENT_STORE_OP_STORE
        VkAttachmentStoreOp stencil_store_op;

        // Default: 0 (no frame pacing)
        uint32_t paced_frame_rate;
    } swapchain;

    // Graphics
    struct
    {
        vkex::PhysicalDeviceFeatures enable_features;
    } graphics;

    // Graphics debug
    struct
    {
        bool                      enable;
        DebugUtilsMessageSeverity message_severity;
        DebugUtilsMessageType     message_type;
    } graphics_debug;

    // ImGui
    bool enable_imgui;

    // Screenshot
    bool enable_screen_shot;
};

/** @class RenderData
 *
 */
class RenderData
{
public:
    RenderData();
    ~RenderData();
    RenderData*                   GetPrevious() const { return m_previous; }
    uint32_t                      GetFrameIndex() const { return m_frame_index; }
    const std::vector<Semaphore>& GetWaitSemaphores() const { return m_wait_semaphores; }
    void                          AddWaitSemaphore(const vkex::Semaphore& semaphore);
    void                          ClearWaitSemaphores();
    vkex::CommandBuffer           GetCommandBuffer() { return m_work_cmd; }
    vkex::Semaphore               GetWorkCompleteSemaphore() const { return m_work_complete_semaphore; }
    vkex::Fence                   GetWorkcompleteFence() const { return m_work_complete_fence; }

private:
    friend class vkex::Application;
    vkex::Result InternalCreate(vkex::Device device, uint32_t frame_index, vkex::CommandBuffer cmd);
    vkex::Result InternalDestroy();
    void         SetPrevious(RenderData* p_previous);

private:
    RenderData*                  m_previous                = nullptr;
    vkex::Device                 m_device                  = nullptr;
    uint32_t                     m_frame_index             = UINT32_MAX;
    std::vector<vkex::Semaphore> m_wait_semaphores         = {};
    vkex::CommandBuffer          m_work_cmd                = nullptr;
    vkex::Semaphore              m_work_complete_semaphore = nullptr;
    vkex::Fence                  m_work_complete_fence     = nullptr;
};

/** @class PresentData
 *o
 */
class PresentData
{
public:
    PresentData();
    ~PresentData();
    PresentData*                  GetPrevious() const { return m_previous; }
    uint32_t                      GetFrameIndex() const { return m_frame_index; }
    vkex::Semaphore               GetImageAcquiredSemaphore() const { return m_image_acquired_sempahore; }
    vkex::Fence                   GetImageAcquiredFence() const { return m_image_acquired_fence; }
    const std::vector<Semaphore>& GetWaitSemaphores() const { return m_wait_semaphores; }
    void                          AddWaitSemaphore(const vkex::Semaphore& semaphore);
    void                          ClearWaitSemaphores();
    vkex::CommandBuffer           GetCommandBuffer() { return m_work_cmd; }
    vkex::Semaphore               GetWorkCompleteForRenderSemaphore() const { return m_work_complete_for_render_semaphore; }
    vkex::Semaphore               GetWorkCompleteForPresentSemaphore() const { return m_work_complete_for_present_semaphore; }
    vkex::Fence                   GetWorkCompleteFence() const { return m_work_complete_fence; }
    vkex::ImageView               GetColorAttachment() const { return m_color_attachment; }
    vkex::ImageView               GetDepthStencilAttachment() const { return m_depth_stencil_attachment; }

private:
    friend class vkex::Application;
    vkex::Result InternalCreate(vkex::Device device, uint32_t frame_index, vkex::CommandBuffer cmd);
    vkex::Result InternalDestroy();
    void         SetAttachments(vkex::ImageView color_attachment, vkex::ImageView depth_stencil_attachment);
    void         SetPrevious(PresentData* p_previous);

private:
    //
    // NOTE: Proper Waiting and Signaling of Semaphores
    //
    // m_work_complete_for_render_semaphore - signaled in frame N
    // to be waited on in frame N+1 in one of the following ways:
    //   - Manually waited on in <YOUR_APP>::Render() if work is
    //     submitted (see 03_render_work).
    //   - Automatically by VKEX in vkex::Application::Present() if
    //     there is NO work submitted in <YOUR_APP>::Render()
    // The intent of this semaphore is to block any render work for
    // the current frame if the previous frame's render work is
    // still on the GPU. Additional note: it's not necessary for
    // the current frame's render work to wait on the previous
    // frame's present work since present is usually asynchronous.
    //
    // m_work_complete_for_present_semaphore - signaed in frame N
    // to be waited in frame N+1. The wait is automatically done
    // done by VKEX. The intent of this semaphore is to block
    // the current frame's present work if the previous frame's
    // present work is still on the GPU.
    //
    //
    PresentData*                 m_previous                            = nullptr;
    vkex::Device                 m_device                              = nullptr;
    uint32_t                     m_frame_index                         = UINT32_MAX;
    vkex::Semaphore              m_image_acquired_sempahore            = nullptr;
    vkex::Fence                  m_image_acquired_fence                = nullptr;
    std::vector<vkex::Semaphore> m_wait_semaphores                     = {};
    vkex::CommandBuffer          m_work_cmd                            = nullptr;
    vkex::Semaphore              m_work_complete_for_render_semaphore  = nullptr;
    vkex::Semaphore              m_work_complete_for_present_semaphore = nullptr;
    vkex::Fence                  m_work_complete_fence                 = nullptr;
    vkex::ImageView              m_color_attachment                    = nullptr;
    vkex::ImageView              m_depth_stencil_attachment            = nullptr;
};

/** @class Application
 *
 */
class Application
{
public:
    Application(const std::string& name = "");
    Application(uint32_t width, uint32_t height, const std::string& name = "");
    virtual ~Application();

    static Application* Get();

    //! @fn Quit
    void Quit();

    //! @fn GetName
    const std::string& GetName() const;

    //! @fn GetPlatformName
    std::string GetPlatformName() const;

    //! @fn GetApplicationMode
    ApplicationMode GetApplicationMode() const;

    //! @fn IsApplicationModeWindow
    bool IsApplicationModeWindow() const;

    //! @fn IsApplicationModeHeadless
    bool IsApplicationModeHeadless() const;

    //! @fn GetProcessId
    uint32_t GetProcessId() const;

    //! @fn GetApplicationPath
    fs::path GetApplicationPath() const;

    //! @fn GetAssetDirs
    const std::vector<fs::path>& GetAssetDirs() const;

    //! @fn GetAssetPath
    fs::path GetAssetPath(const std::string& file_name) const;

    //! @fn AddAssetDir
    void AddAssetDir(const fs::path& path);

    virtual void AddArgs(vkex::ArgParser& args) {}
    virtual void Configure(const vkex::ArgParser& args, vkex::Configuration& configuration) {}
    virtual void Setup() {}
    virtual void Destroy() {}
    virtual void Move(int32_t x, int32_t y) {}
    virtual void Resize(uint32_t width, uint32_t height) {}
    virtual void MouseDown(int32_t x, int32_t y, uint32_t buttons) {}
    virtual void MouseUp(int32_t x, int32_t y, uint32_t buttons) {}
    virtual void MouseMove(int32_t x, int32_t y, uint32_t buttons) {}
    virtual void KeyUp(KeyboardInput key) {}
    virtual void KeyDown(KeyboardInput key) {}
    virtual void Update(double frame_elapsed_time) {}
    virtual void Render(RenderData* p_current_render_data, PresentData* p_current_present_data) {}
    virtual void Present(PresentData* p_current_render_data){};

    // Dispatchers - override these to change the call sequence
    virtual void DispatchCallAddArgs(vkex::ArgParser& args);
    virtual void DispatchCallConfigure(const vkex::ArgParser& args, vkex::Configuration& configuration);
    virtual void DispatchCallSetup();
    virtual void DispatchCallDestroy();
    virtual void DispatchCallMove(int32_t x, int32_t y);
    virtual void DispatchCallResize(uint32_t width, uint32_t height);
    virtual void DispatchCallMouseDown(int32_t x, int32_t y, uint32_t buttons);
    virtual void DispatchCallMouseUp(int32_t x, int32_t y, uint32_t buttons);
    virtual void DispatchCallMouseMove(int32_t x, int32_t y, uint32_t buttons);
    virtual void DispatchCallKeyUp(KeyboardInput key);
    virtual void DispatchCallKeyDown(KeyboardInput key);
    virtual void DispatchCallUpdate(double frame_elapsed_time);
    virtual void DispatchCallRender(RenderData* p_render_data, PresentData* p_present_data);
    virtual void DispatchCallPresent(PresentData* p_present_data);

    //! @fn IsKeyPressed
    bool IsKeyPressed(KeyboardInput key);

    //! @fn IsMouseDown
    bool IsMouseDown(int button) const;

    //! @fn GetCursorMode
    vkex::CursorMode GetCursorMode() const
    {
        return m_configuration.window.cursor_mode;
    }

    //! @fn SetCursorMode
    void SetCursorMode(vkex::CursorMode cursor_mode);

    //! @fn DrawImGui
    void DrawImGui(vkex::CommandBuffer cmd);

    //! @fn SubmitPresent
    vkex::Result SubmitRender(vkex::RenderData* p_current_render_data, vkex::PresentData* p_current_present_data);

    //! @fn SubmitPresent
    vkex::Result SubmitPresent(vkex::PresentData* p_current_render_data);

    //! @fn Run
    vkex::Result Run(int argn, const char* const* argv);

    //! @fn GetConfiguration
    const vkex::Configuration& GetConfiguration() const;

    //! @fn GetDevice
    vkex::Device GetDevice() const
    {
        return m_device;
    }

    //! @fn GetWindowWidth
    uint32_t GetWindowWidth() const
    {
        return m_configuration.window.width;
    }
    //! @fn GetWindowHeight
    uint32_t GetWindowHeight() const
    {
        return m_configuration.window.height;
    }
    //! @fn GetWindowAspect
    float GetWindowAspect() const
    {
        if (m_configuration.window.height == 0) {
            return 1.0f;
        }
        float aspect = static_cast<float>(m_configuration.window.width) / static_cast<float>(m_configuration.window.height);
        return aspect;
    }

    //! @fn GetFrameCount - Returns the number of frames in flight.
    uint32_t GetFrameCount() const;
    //! @fn GetCurrentFrameIndex - Returns the current index of the current frame in flight.
    uint32_t GetCurrentFrameIndex() const;

    //! @fn GetElapsedFrames - Returns the number of elapsed frames since the application started.
    uint64_t GetElapsedFrames() const
    {
        return m_elapsed_frame_count;
    }
    //! @fn GetElapsedTime - Returns the elapsed seconds since the application started.
    float GetElapsedTime() const;
    //! @fn GetFrameStartTime - Returns the current frame's start time in seconds.
    float GetFrameStartTime() const
    {
        return static_cast<float>(m_frame_start_time);
    }
    //! @fn GetAverageFrameSeconds - Returns the average frame time in seconds
    float GetAverageFrameTime() const
    {
        return static_cast<float>(m_average_frame_time);
    }
    //! @fn GetFramesPerSecond - Returns the current frame rate in FPS.
    float GetFramesPerSecond() const
    {
        return static_cast<float>(m_frames_per_second);
    }
    //! @fn GetFrameElapsedTime - Returns the current frame elapsed time in seconds
    float GetFrameElapsedTime() const
    {
        return static_cast<float>(m_frame_elapsed_time);
    }
    //! @fn GetMaxWindowFrameTime - Returns the maximum GetFrameElapsedTime() observed so far in the time window
    float GetMaxWindowFrameTime() const
    {
        return static_cast<float>(m_max_window_frame_time);
    }
    //! @fn GetMinWindowFrameTime - Returns the minimum GetFrameElapsedTime() observed so far in the time window
    float GetMinWindowFrameTime() const
    {
        return static_cast<float>(m_min_window_frame_time);
    }

    //! @fn SetSwapchainFormat
    void SetSwapchainFormat(VkFormat format, VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

    //! @fn SetSwapchainPresentMode
    void SetSwapchainPresentMode(VkPresentModeKHR present_mode);

    //! @fn GetGraphicsQueue
    vkex::Queue GetGraphicsQueue() const;

    //! @fn GetComputeQueue
    vkex::Queue GetComputeQueue() const;

    //! @fn GetCurrentRenderData()
    vkex::RenderData* GetCurrentRenderData() const;

    //! @fn GetCurrentPresentData()
    vkex::PresentData* GetCurrentPresentData() const;

    //! @fn GetAverageVkQueuePresentTime
    float GetAverageVkQueuePresentTime() const
    {
        return m_average_vk_queue_present_time;
    }

    //! @fn DrawDebugApplicationInfo
    void DrawDebugApplicationInfo();

private:
    friend struct WindowEvents;

    //! @fn InitializeAssetDirs
    void InitializeAssetDirs();

    //! @fn InitializeWindow
    vkex::Result InitializeWindow();

    //! @fn InitializeVkexDevice
    vkex::Result InitializeVkexDevice();

    //! @fn InitializeVkexSwapchainImageMemoryPool();
    vkex::Result InitializeVkexSwapchainImageMemoryPool();

    //! @fn InitializeVkexSwapchain
    vkex::Result InitializeVkexSwapchain();

    //! @fn InitializeVkexSwapchain
    vkex::Result InitializeFakeSwapchain();

    //! @fn InitializeVkexPerFrameRenderData();
    vkex::Result InitializeVkexPerFrameRenderData();

    //! @fn InitializeVkexPerFramePresentData();
    vkex::Result InitializeVkexPerFramePresentData();

    //! @fn InitializeVkex
    vkex::Result InitializeVkex();

    //! @fn InitializeImgui
    vkex::Result InitializeImGui();

    //! @fn InternalCreate
    vkex::Result InternalCreate();

    //! @fn DestroyVkexSwapchain();
    vkex::Result DestroyVkexSwapchainImageMemoryPool();

    //! @fn DestroyVkexSwapchain();
    vkex::Result DestroyVkexSwapchain();

    //! @fn DestroyFakeSwapchain();
    vkex::Result DestroyFakeSwapchain();

    //! @fn InitializeImgui
    vkex::Result DestroyImGui();

    //! @fn InternalDestroy
    vkex::Result InternalDestroy();

    //! @fn RecreateVkexSwapchain();
    vkex::Result RecreateVkexSwapchain();

    //! @fn MoveCallback
    void MoveCallback(int32_t x, int32_t y);
    //! @fn ResizeCallback
    void ResizeCallback(uint32_t width, uint32_t height);
    //! @fn MouseDownCallback
    void MouseDownCallback(int32_t x, int32_t y, uint32_t buttons);
    //! @fn MouseUpCallback
    void MouseUpCallback(int32_t x, int32_t y, uint32_t buttons);
    //! @fn MouseMoveCallback
    void MouseMoveCallback(int32_t x, int32_t y, uint32_t buttons);
    //! @fn  KeyUpCallback
    void KeyUpCallback(KeyboardInput key);
    //! @fn  KeyDownCallback
    void KeyDownCallback(KeyboardInput key);

    //! @fn CheckConfiguration
    vkex::Result CheckConfiguration();

    //! @fn UpdateCurrentPerFrameData
    vkex::Result UpdateCurrentPerFrameData();

    //! @fn ProcessRenderFence
    vkex::Result ProcessRenderFence(vkex::RenderData* p_data);

    //! @fn ProcessFrameFence
    vkex::Result ProcessFrameFence(vkex::PresentData* p_data);

    //! @fn SubmitPresent
    vkex::Result AcquireNextImage(vkex::PresentData* p_data, uint32_t* p_swapchain_image_index);

    //! @fn WaitAllQueuesIdle
    vkex::Result WaitAllQueuesIdle();

private:
    bool IsRunning() const;

protected:
    bool m_running = false;

    vkex::ArgParser       m_args;
    vkex::Configuration   m_configuration = {};
    std::vector<fs::path> m_asset_dirs;

    GLFWwindow* m_window                 = nullptr;
    bool        m_loop_started           = false;
    double      m_frame_start_time       = 0;
    double      m_average_frame_time     = 0;
    double      m_frame_start_time_delta = 0;
    double      m_frames_per_second      = 0;
    double      m_total_frame_time       = 0;
    double      m_frame_elapsed_time     = 0;

    double m_update_fn_time  = 0;
    double m_render_fn_time  = 0;
    double m_present_fn_time = 0;

    const uint32_t kWindowFrames           = 100;
    uint32_t       m_window_frame_count    = kWindowFrames;
    double         m_max_window_frame_time = 0;
    double         m_min_window_frame_time = std::numeric_limits<double>::max();

    vkex::Instance               m_instance                      = nullptr;
    vkex::Device                 m_device                        = nullptr;
    vkex::Queue                  m_graphics_queue                = nullptr;
    vkex::Queue                  m_compute_queue                 = nullptr;
    vkex::Queue                  m_transfer_queue                = nullptr;
    vkex::Queue                  m_present_queue                 = nullptr;
    vkex::Surface                m_surface                       = nullptr;
    VmaPool                      m_swapchain_image_memory_pool   = VK_NULL_HANDLE;
    vkex::Swapchain              m_swapchain                     = nullptr;
    uint32_t                     m_current_swapchain_image_index = UINT32_MAX;
    std::vector<vkex::ImageView> m_swapchain_color_image_views;
    std::vector<vkex::ImageView> m_swapchain_depth_stencil_image_views;
    std::vector<vkex::Image>     m_fake_swapchain_color_images;
    std::vector<vkex::Image>     m_fake_swapchain_depth_stencil_images;
    std::vector<vkex::ImageView> m_fake_swapchain_color_image_views;
    std::vector<vkex::ImageView> m_fake_swapchain_depth_stencil_image_views;
    uint64_t                     m_elapsed_frame_count    = 0;
    uint32_t                     m_frame_index            = 0;
    bool                         m_recreate_swapchain     = false;
    bool                         m_window_surface_invalid = false;

    double m_frame_0_time = 0;

    using RenderDataPtr = std::unique_ptr<RenderData>;
    std::vector<RenderDataPtr> m_per_frame_render_data;
    std::vector<RenderData*>   m_render_data_stack;
    vkex::CommandPool          m_render_command_pool = nullptr;
    bool                       m_render_submitted    = false;
    RenderData*                m_current_render_data = nullptr;

    using PresentDataPtr = std::unique_ptr<PresentData>;
    std::vector<PresentDataPtr> m_per_frame_present_data;
    std::vector<PresentData*>   m_present_data_stack;
    vkex::CommandPool           m_present_command_pool = nullptr;
    PresentData*                m_current_present_data = nullptr;
    // PresentData*                  m_previous_present_data = nullptr;

    vkex::DescriptorPool m_imgui_descriptor_pool = nullptr;

    bool m_keys[kNumKeys] = {false};

    bool         m_screen_shot       = false;
    vkex::Buffer m_screenshot_buffer = nullptr;

    HistoryT<TimeRange, 100> m_vk_queue_present_times;
    float                    m_average_vk_queue_present_time = 0;
};

} // namespace vkex

#endif // __VKEX_APPLICATION_H__