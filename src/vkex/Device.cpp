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

#include "vkex/Device.h"
#include "vkex/Instance.h"
#include "vkex/Log.h"

#include "vkex/ToString.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <map>

namespace vkex {

PFN_vkCmdPushDescriptorSetKHR CmdPushDescriptorSetKHR = nullptr;

PFN_vkGetDescriptorSetLayoutSizeEXT                          GetDescriptorSetLayoutSizeEXT                          = nullptr;
PFN_vkGetDescriptorSetLayoutBindingOffsetEXT                 GetDescriptorSetLayoutBindingOffsetEXT                 = nullptr;
PFN_vkGetDescriptorEXT                                       GetDescriptorEXT                                       = nullptr;
PFN_vkCmdBindDescriptorBuffersEXT                            CmdBindDescriptorBuffersEXT                            = nullptr;
PFN_vkCmdSetDescriptorBufferOffsetsEXT                       CmdSetDescriptorBufferOffsetsEXT                       = nullptr;
PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT             CmdBindDescriptorBufferEmbeddedSamplersEXT             = nullptr;
PFN_vkGetBufferOpaqueCaptureDescriptorDataEXT                GetBufferOpaqueCaptureDescriptorDataEXT                = nullptr;
PFN_vkGetImageOpaqueCaptureDescriptorDataEXT                 GetImageOpaqueCaptureDescriptorDataEXT                 = nullptr;
PFN_vkGetImageViewOpaqueCaptureDescriptorDataEXT             GetImageViewOpaqueCaptureDescriptorDataEXT             = nullptr;
PFN_vkGetSamplerOpaqueCaptureDescriptorDataEXT               GetSamplerOpaqueCaptureDescriptorDataEXT               = nullptr;
PFN_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT GetAccelerationStructureOpaqueCaptureDescriptorDataEXT = nullptr;

static void WireUpPNexts(vkex::PhysicalDeviceFeatures& features)
{
    features.bufferDeviceAddress.pNext       = nullptr;
    features.descriptorIndexing.pNext        = &features.bufferDeviceAddress;
    features.ext.depthClampZeroOne.pNext     = &features.descriptorIndexing;
    features.ext.depthClipControl.pNext      = &features.ext.depthClampZeroOne;
    features.ext.depthClipEnable.pNext       = &features.ext.depthClipControl;
    features.ext.descriptorBuffer.pNext      = &features.ext.depthClipEnable;
    features.ext.extendedDynamicState3.pNext = &features.ext.descriptorBuffer;
    features.khr.dynamicRendering.pNext      = &features.ext.extendedDynamicState3;
    features.khr.synchronization2.pNext      = &features.khr.dynamicRendering;
    features.khr.timelineSemaphore.pNext     = &features.khr.synchronization2;
    features.khr.rayTracingPipeline.pNext    = &features.khr.timelineSemaphore;
    features.khr.accelerationStructure.pNext = &features.khr.rayTracingPipeline;
    features.pFirst                          = &features.khr.accelerationStructure;
}

static void WireUpPNexts(vkex::PhysicalDeviceProperties& properties)
{
    properties.descriptorIndexing.pNext   = nullptr;
    properties.ext.descriptorBuffer.pNext = &properties.descriptorIndexing;
    properties.khr.pushDescriptor.pNext   = &properties.ext.descriptorBuffer;
    properties.pFirst                     = &properties.khr.pushDescriptor;
}

static void ClearPNext(vkex::PhysicalDeviceFeatures& features)
{
    features.bufferDeviceAddress.pNext       = nullptr;
    features.descriptorIndexing.pNext        = nullptr;
    features.ext.depthClampZeroOne.pNext     = nullptr;
    features.ext.depthClipControl.pNext      = nullptr;
    features.ext.depthClipEnable.pNext       = nullptr;
    features.ext.descriptorBuffer.pNext      = nullptr;
    features.ext.extendedDynamicState3.pNext = nullptr;
    features.khr.dynamicRendering.pNext      = nullptr;
    features.khr.synchronization2.pNext      = nullptr;
    features.khr.timelineSemaphore.pNext     = nullptr;
    features.khr.rayTracingPipeline.pNext    = nullptr;
    features.khr.accelerationStructure.pNext = nullptr;
    features.pFirst                          = nullptr;
}

static void ClearPNext(vkex::PhysicalDeviceProperties& properties)
{
    properties.descriptorIndexing.pNext = nullptr;
    properties.khr.pushDescriptor.pNext = nullptr;
    properties.pFirst                   = nullptr;
}

static void SetStructureTypes(vkex::PhysicalDeviceFeatures& features)
{
    features.bufferDeviceAddress.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    features.descriptorIndexing.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    features.ext.depthClampZeroOne.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT;
    features.ext.depthClipControl.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT;
    features.ext.depthClipEnable.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    features.ext.descriptorBuffer.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
    features.ext.extendedDynamicState3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    features.khr.dynamicRendering.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    features.khr.synchronization2.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    features.khr.timelineSemaphore.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    features.khr.rayTracingPipeline.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    features.khr.accelerationStructure.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
}

static void SetStructureTypes(vkex::PhysicalDeviceProperties& properties)
{
    properties.descriptorIndexing.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
    properties.ext.descriptorBuffer.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
    properties.khr.pushDescriptor.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
}

static VkResult EnumerateDeviceExtensionNames(VkPhysicalDevice vk_physical_device, std::vector<std::string>& found_extensions)
{
    uint32_t count     = 0;
    VkResult vk_result = InvalidValue<VkResult>::Value;
    VKEX_VULKAN_RESULT_CALL(
        vk_result,
        vkEnumerateDeviceExtensionProperties(
            vk_physical_device,
            nullptr,
            &count,
            nullptr));
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }

    std::vector<VkExtensionProperties> properties_list(count);
    VKEX_VULKAN_RESULT_CALL(
        vk_result,
        vkEnumerateDeviceExtensionProperties(
            vk_physical_device,
            nullptr,
            &count,
            vkex::DataPtr(properties_list)));
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }

    // Copy extension names
    for (auto& properties : properties_list) {
        std::string name  = properties.extensionName;
        bool        found = Contains(found_extensions, name);
        // Skip adding if extension name already exists
        if (found) {
            continue;
        }
        found_extensions.push_back(name);
    }

    return VK_SUCCESS;
}

// =================================================================================================
// PhysicalDevice
// =================================================================================================
CPhysicalDevice::CPhysicalDevice()
{
}

CPhysicalDevice::~CPhysicalDevice()
{
}

vkex::Result CPhysicalDevice::InternalCreate(
    const vkex::PhysicalDeviceCreateInfo& create_info,
    const VkAllocationCallbacks*          p_allocator)
{
    // Copy create info
    m_create_info = create_info;

    // Properties
    {
        m_physical_device_properties = {};
        WireUpPNexts(m_physical_device_properties);
        SetStructureTypes(m_physical_device_properties);

        VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        properties2.pNext                       = m_physical_device_properties.pFirst;

        vkGetPhysicalDeviceProperties2(
            m_create_info.vk_object,
            &properties2);

        m_physical_device_properties.core = properties2.properties;
    }

    // Descriptive name
    InitializeVendorProperties();

    // Features
    {
        m_physical_device_features = {};
        WireUpPNexts(m_physical_device_features);
        SetStructureTypes(m_physical_device_features);

        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features2.pNext                     = m_physical_device_features.pFirst;

        vkGetPhysicalDeviceFeatures2(
            m_create_info.vk_object,
            &features2);

        m_physical_device_features.core = features2.features;

        std::vector<std::string> found_extensions;
        auto                     vk_result = vkex::EnumerateDeviceExtensionNames(
            m_create_info.vk_object,
            found_extensions);
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }

        m_physical_device_features.ext.loadStoreOpNone = Contains(found_extensions, std::string(VK_EXT_LOAD_STORE_OP_NONE_EXTENSION_NAME));
        m_physical_device_features.khr.pushDescriptor  = Contains(found_extensions, std::string(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME));
    }

    // Queue family properties
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(
            m_create_info.vk_object,
            &count,
            nullptr);

        for (uint32_t i = 0; i < count; ++i) {
            m_vk_queue_family_properties.push_back({VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
        }

        vkGetPhysicalDeviceQueueFamilyProperties2(
            m_create_info.vk_object,
            &count,
            vkex::DataPtr(m_vk_queue_family_properties));
    }

    // Memory properties
    {
        m_vk_physical_device_memory_properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
        vkGetPhysicalDeviceMemoryProperties2(
            m_create_info.vk_object,
            &m_vk_physical_device_memory_properties);
    }

    return vkex::Result::Success;
}

vkex::Result CPhysicalDevice::InternalDestroy(const VkAllocationCallbacks* p_allocator)
{
    return vkex::Result::Success;
}

void CPhysicalDevice::InitializeVendorProperties()
{
    //
    // NOTE: This function uses vk* functions since the pointers
    //       will not be completely loaded.
    //

    VkPhysicalDeviceProperties vk_physical_device_properties = {};
    vkGetPhysicalDeviceProperties(m_create_info.vk_object, &vk_physical_device_properties);

    std::stringstream ss;
    ss << vk_physical_device_properties.deviceName;

    if (vk_physical_device_properties.vendorID == VKEX_IHV_VENDOR_ID_AMD) {
        const char* extension = VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME;

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_create_info.vk_object, &count, nullptr);

        std::vector<VkQueueFamilyProperties> vk_queue_family_properties_list(count);
        if (count > 0) {
            vkGetPhysicalDeviceQueueFamilyProperties(m_create_info.vk_object, &count, vk_queue_family_properties_list.data());
        }

        uint32_t graphics_queue_family_index = UINT32_MAX;
        for (uint32_t queue_family_index = 0; queue_family_index < count; ++queue_family_index) {
            vkex::QueueFlags queue_flags = {};
            queue_flags.flags            = vk_queue_family_properties_list[queue_family_index].queueFlags;
            if (queue_flags.bits.graphics) {
                graphics_queue_family_index = queue_family_index;
                break;
            }
        }

        if (graphics_queue_family_index != UINT32_MAX) {
            const float             k_queue_priority     = 1.0f;
            VkDeviceQueueCreateInfo vk_queue_create_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            vk_queue_create_info.queueFamilyIndex        = graphics_queue_family_index;
            vk_queue_create_info.queueCount              = 1;
            vk_queue_create_info.pQueuePriorities        = &k_queue_priority;

            VkDeviceCreateInfo vk_device_create_info      = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            vk_device_create_info.queueCreateInfoCount    = 1;
            vk_device_create_info.pQueueCreateInfos       = &vk_queue_create_info;
            vk_device_create_info.enabledExtensionCount   = 1;
            vk_device_create_info.ppEnabledExtensionNames = &extension;

            VkDevice vk_device = VK_NULL_HANDLE;
            VkResult vk_result = vkCreateDevice(m_create_info.vk_object, &vk_device_create_info, nullptr, &vk_device);
            if (vk_result == VK_SUCCESS) {
                m_vendor_properties.amd.shader_core_properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD};

                VkPhysicalDeviceProperties2 properties_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
                properties_2.pNext                       = &m_vendor_properties.amd.shader_core_properties;

                vkGetPhysicalDeviceProperties2(m_create_info.vk_object, &properties_2);

                uint32_t cu_count = m_vendor_properties.amd.shader_core_properties.shaderEngineCount *
                                    m_vendor_properties.amd.shader_core_properties.shaderArraysPerEngineCount *
                                    m_vendor_properties.amd.shader_core_properties.computeUnitsPerShaderArray;
                ss << " " << cu_count;

                vkDestroyDevice(vk_device, nullptr);
            }
        }
    }

    m_descriptive_name = ss.str();
}

bool CPhysicalDevice::GetQueueFamilyProperties(
    uint32_t                  vk_queue_family_index,
    VkQueueFamilyProperties2* p_vk_queue_family_properties) const
{
    uint32_t count = CountU32(m_vk_queue_family_properties);
    if (vk_queue_family_index >= count) {
        return false;
    }

    *p_vk_queue_family_properties = m_vk_queue_family_properties[vk_queue_family_index];

    return true;
}

VkBool32 CPhysicalDevice::SupportsPresent(uint32_t queue_family_index, const vkex::DisplayInfo& display_info) const
{
#if defined(VKEX_LINUX_WAYLAND)
    VkBool32 supported = vkex::GetPhysicalDeviceWaylandPresentationSupportKHR(
        m_create_info.vk_object,
        queue_family_index,
        display_info.display);
#elif defined(VKEX_LINUX_XCB)
    VkBool32 supported = vkex::GetPhysicalDeviceXcbPresentationSupportKHR(
        m_create_info.vk_object,
        queue_family_index,
        display_info.connection,
        display_info.visual_id);
#elif defined(VKEX_LINUX_XLIB)
    VkBool32 supported = vkex::GetPhysicalDeviceXlibPresentationSupportKHR(
        m_create_info.vk_object,
        queue_family_index,
        display_info.dpy,
        display_info.visual_id);
#elif defined(VKEX_WIN32)
    VkBool32 supported = vkGetPhysicalDeviceWin32PresentationSupportKHR(
        m_create_info.vk_object,
        queue_family_index);
#endif
    return supported;
}

// =================================================================================================
// Device
// =================================================================================================
CDevice::CDevice()
{
}

CDevice::~CDevice()
{
}

vkex::Result CDevice::InitializeExtensions()
{
    // Enumerate extensions
    {
        VkPhysicalDevice vk_physical_device = m_create_info.physical_device->GetVkObject();

        VkResult vk_result = vkex::EnumerateDeviceExtensionNames(
            vk_physical_device,
            m_found_extensions);
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }
    }

    // Replace layers if specified in the Vulkan create info
    if (m_vk_create_info.enabledExtensionCount > 0) {
        m_create_info.extensions.clear();
        uint32_t count = m_vk_create_info.enabledExtensionCount;
        for (uint32_t i = 0; i < count; ++i) {
            std::string name = m_vk_create_info.ppEnabledExtensionNames[i];
            m_create_info.extensions.push_back(name);
        }
    }

    // Enable extensions
    {
        std::vector<std::string> enabled_extensions = {
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
        };

        // EXT
        {
            if (m_create_info.enabled_features.ext.descriptorBuffer.descriptorBuffer || m_create_info.enabled_features.ext.descriptorBuffer.descriptorBufferPushDescriptors) {
                enabled_extensions.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
            }
            if (m_create_info.enabled_features.ext.loadStoreOpNone) {
                enabled_extensions.push_back(VK_EXT_LOAD_STORE_OP_NONE_EXTENSION_NAME);
            }
        }

        // KHR
        {
            if (m_create_info.enabled_features.khr.pushDescriptor) {
                enabled_extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            }
            if (m_create_info.enabled_features.khr.rayTracingPipeline.rayTracingPipeline) {
                enabled_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            }
            if (m_create_info.enabled_features.khr.accelerationStructure.accelerationStructure) {
                enabled_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            }
        }

        if (GetInstance()->IsSwapchainEnabled()) {
            m_create_info.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (m_create_info.physical_device->IsAMD()) {
            m_create_info.extensions.push_back(VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME);
        }
        else {
            VKEX_LOG_WARN("Skipping AMD extension on non-AMD device: " << VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME);
        }

        for (auto& name : enabled_extensions) {
            // Check to make sure extension is available
            bool found = Contains(m_found_extensions, name);
            if (!found) {
                VKEX_ASSERT_MSG(found, "Required device extension not found: " << name);
                return vkex::Result::ErrorDeviceExtensionNotFound;
            }
            // Skip adding if extension name already exists
            bool already_exists = Contains(m_create_info.extensions, name);
            if (already_exists) {
                continue;
            }
            m_create_info.extensions.push_back(name);
        }
    }

    // Check extension names
    for (auto& name : m_create_info.extensions) {
        bool found = Contains(m_found_extensions, name);
        // Bail if requested layer isn't found
        if (!found) {
            return vkex::Result::ErrorDeviceExtensionNotFound;
        }
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::InitializeQueueRequests()
{
    // Fill any missing queue priorities with 1.0
    for (auto& create_info : m_create_info.queue_create_infos) {
        while (create_info.queue_priorities.size() < create_info.queue_count) {
            create_info.queue_priorities.push_back(1.0f);
        }
    }

    for (const auto& create_info : m_create_info.queue_create_infos) {
        // Check priority count
        uint32_t priority_count = CountU32(create_info.queue_priorities);
        if (create_info.queue_count > priority_count) {
            return vkex::Result::ErrorInvalidQueuePriorityCount;
        }
        // Device queue create info
        VkDeviceQueueCreateInfo vk_create_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        vk_create_info.flags                   = 0;
        vk_create_info.queueFamilyIndex        = create_info.queue_family_index;
        vk_create_info.queueCount              = create_info.queue_count;
        vk_create_info.pQueuePriorities        = DataPtr(create_info.queue_priorities);
        m_vk_queue_create_infos.push_back(vk_create_info);
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::InitializeQueues()
{
    std::vector<const void*> look_up_keys;
    for (const auto& create_info : m_create_info.queue_create_infos) {
        // Look for queue family properties
        VkQueueFamilyProperties2 vk_queue_family_properties = {};
        bool                     found                      = m_create_info.physical_device->GetQueueFamilyProperties(
            create_info.queue_family_index,
            &vk_queue_family_properties);
        VKEX_ASSERT_MSG(
            found,
            "Unable to find queue family properties for queue family " << create_info.queue_family_index);
        if (!found) {
            return vkex::Result::ErrorInvalidQueueFamilyIndex;
        }
        // Create queue slots
        for (uint32_t queue_index = 0; queue_index < create_info.queue_count; ++queue_index) {
            //
            // Use vkGetDeviceQueue and not vkGetDeviceQueue2 since there no need to
            // support VkDeviceQueueCreateFlags yet.
            //
            VkQueue vk_queue = VK_NULL_HANDLE;
            vkGetDeviceQueue(
                m_vk_object,
                create_info.queue_family_index,
                queue_index,
                &vk_queue);

            VKEX_ASSERT_MSG(
                (vk_queue != VK_NULL_HANDLE),
                "Unable to retrieve device queue for queue family " << create_info.queue_family_index << " at queue index " << queue_index);

            // Add lookup key
            look_up_keys.push_back(vk_queue);

            // Supported queue flags
            VkQueueFlags vk_supported_queue_flags = static_cast<VkQueueFlags>(0);
            switch (create_info.queue_type) {
                default: VKEX_ASSERT_MSG(false, "Unsupport queue type " << create_info.queue_type); break;

                // Graphics
                case VK_QUEUE_GRAPHICS_BIT: {
                    vk_supported_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
                } break;
                // Compute
                case VK_QUEUE_COMPUTE_BIT: {
                    vk_supported_queue_flags = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
                } break;
                // Transfer
                case VK_QUEUE_TRANSFER_BIT: {
                    vk_supported_queue_flags = VK_QUEUE_TRANSFER_BIT;
                } break;
            }

            // Create VKEX queue object
            vkex::QueueCreateInfo queue_create_info       = {};
            queue_create_info.requested_queue_flags.flags = create_info.queue_type;
            queue_create_info.supported_queue_flags.flags = vk_supported_queue_flags;
            queue_create_info.queue_family_index          = create_info.queue_family_index;
            queue_create_info.queue_index                 = queue_index;
            queue_create_info.vk_object                   = vk_queue;
            vkex::Queue  queue                            = nullptr;
            vkex::Result vkex_result                      = CreateObject<CQueue>(
                queue_create_info,
                nullptr,
                m_stored_queues,
                &CQueue::SetDevice,
                this,
                &queue);
            if (!vkex_result) {
                return vkex_result;
            }
        }
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::InternalCreate(
    const vkex::DeviceCreateInfo& create_info,
    const VkAllocationCallbacks*  p_allocator)
{
    // Copy create info
    m_create_info = create_info;

    // Check Vulkan API version number - VKEX requires at least Vulkan 1.1
    {
        uint32_t api_version    = m_create_info.physical_device->GetApiVersion();
        int      major          = VK_VERSION_MAJOR(api_version);
        int      minor          = VK_VERSION_MINOR(api_version);
        bool     is_valid_major = (major >= 1);
        bool     is_valid_minor = (minor >= 1);
        if (!(is_valid_major && is_valid_minor)) {
            return vkex::Result::ErrorVkexRequiresAtLeastVulkan11;
        }
    }

    // Features
    {
        vkex::WireUpPNexts(m_create_info.enabled_features);
        vkex::SetStructureTypes(m_create_info.enabled_features);

        // Force core features
        m_create_info.enabled_features.core.geometryShader          = VK_TRUE;
        m_create_info.enabled_features.core.tessellationShader      = VK_TRUE;
        m_create_info.enabled_features.core.dualSrcBlend            = VK_TRUE;
        m_create_info.enabled_features.core.occlusionQueryPrecise   = VK_TRUE;
        m_create_info.enabled_features.core.pipelineStatisticsQuery = VK_TRUE;
        m_create_info.enabled_features.core.samplerAnisotropy       = VK_TRUE;
        // Force KHR features
        m_create_info.enabled_features.khr.dynamicRendering.dynamicRendering   = VK_TRUE;
        m_create_info.enabled_features.khr.synchronization2.synchronization2   = VK_TRUE;
        m_create_info.enabled_features.khr.timelineSemaphore.timelineSemaphore = VK_TRUE;

        // Enable all of these if one is enabled
        if (m_create_info.enabled_features.khr.rayTracingPipeline.rayTracingPipeline ||
            m_create_info.enabled_features.khr.accelerationStructure.accelerationStructure) {
            m_create_info.enabled_features.khr.rayTracingPipeline.rayTracingPipeline       = VK_TRUE;
            m_create_info.enabled_features.khr.accelerationStructure.accelerationStructure = VK_TRUE;
        }

        // Enable buffer device address and descriptor indexing if these are enabled
        if (m_create_info.enabled_features.ext.descriptorBuffer.descriptorBuffer ||
            m_create_info.enabled_features.khr.rayTracingPipeline.rayTracingPipeline) {
            // Buffer device address
            m_create_info.enabled_features.bufferDeviceAddress.bufferDeviceAddress = VK_TRUE;
            // Use descriptor indexing properties from physical device
            auto& physical_device_features                    = m_create_info.physical_device->GetPhysicalDeviceFeatures();
            m_create_info.enabled_features.descriptorIndexing = physical_device_features.descriptorIndexing;
        }
    }

    // Initialize extensions
    {
        vkex::Result vkex_result = InitializeExtensions();
        if (!vkex_result) {
            return vkex_result;
        }
    }

    // Initialize queue requests
    {
        vkex::Result vkex_result = InitializeQueueRequests();
        if (!vkex_result) {
            return vkex_result;
        }
    }

    // Create info
    {
        m_c_str_extensions = GetCStrings(m_create_info.extensions);

        m_vk_create_info                         = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        m_vk_create_info.pNext                   = m_create_info.enabled_features.pFirst;
        m_vk_create_info.flags                   = 0;
        m_vk_create_info.queueCreateInfoCount    = CountU32(m_vk_queue_create_infos);
        m_vk_create_info.pQueueCreateInfos       = DataPtr(m_vk_queue_create_infos);
        m_vk_create_info.enabledLayerCount       = 0;
        m_vk_create_info.ppEnabledLayerNames     = nullptr;
        m_vk_create_info.enabledExtensionCount   = CountU32(m_c_str_extensions);
        m_vk_create_info.ppEnabledExtensionNames = DataPtr(m_c_str_extensions);
        m_vk_create_info.pEnabledFeatures        = &m_create_info.enabled_features.core;
    }

    VKEX_LOG_INFO(ToString(m_vk_create_info));

    // Create Vulkan instance
    {
        VkResult vk_result = InvalidValue<VkResult>::Value;
        VKEX_VULKAN_RESULT_CALL(
            vk_result,
            vkCreateDevice(
                *m_create_info.physical_device,
                &m_vk_create_info,
                p_allocator,
                &m_vk_object));
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }
    }

    // Load functions
    {
        CmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(m_vk_object, "vkCmdPushDescriptorSetKHR");

        GetDescriptorSetLayoutSizeEXT                          = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetDescriptorSetLayoutSizeEXT");
        GetDescriptorSetLayoutBindingOffsetEXT                 = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetDescriptorSetLayoutBindingOffsetEXT");
        GetDescriptorEXT                                       = (PFN_vkGetDescriptorEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetDescriptorEXT");
        CmdBindDescriptorBuffersEXT                            = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(m_vk_object, "vkCmdBindDescriptorBuffersEXT");
        CmdSetDescriptorBufferOffsetsEXT                       = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(m_vk_object, "vkCmdSetDescriptorBufferOffsetsEXT");
        CmdBindDescriptorBufferEmbeddedSamplersEXT             = (PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT)vkGetDeviceProcAddr(m_vk_object, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT");
        GetBufferOpaqueCaptureDescriptorDataEXT                = (PFN_vkGetBufferOpaqueCaptureDescriptorDataEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetBufferOpaqueCaptureDescriptorDataEXT");
        GetImageOpaqueCaptureDescriptorDataEXT                 = (PFN_vkGetImageOpaqueCaptureDescriptorDataEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetImageOpaqueCaptureDescriptorDataEXT");
        GetImageViewOpaqueCaptureDescriptorDataEXT             = (PFN_vkGetImageViewOpaqueCaptureDescriptorDataEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetImageViewOpaqueCaptureDescriptorDataEXT");
        GetSamplerOpaqueCaptureDescriptorDataEXT               = (PFN_vkGetSamplerOpaqueCaptureDescriptorDataEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetSamplerOpaqueCaptureDescriptorDataEXT");
        GetAccelerationStructureOpaqueCaptureDescriptorDataEXT = (PFN_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT)vkGetDeviceProcAddr(m_vk_object, "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT");
    }

    // Log device creation
    {
        uint32_t version = m_create_info.physical_device->GetApiVersion();
        int      major   = VK_VERSION_MAJOR(version);
        int      minor   = VK_VERSION_MINOR(version);
        VKEX_LOG_INFO("");
        VKEX_LOG_INFO("Vulkan " << major << "." << minor << " device created (object=VkDevice)");
        // Device properties
        {
            auto& properties = m_create_info.physical_device->GetPhysicalDeviceProperties().core;
            VKEX_LOG_INFO("Device properties:");
            VKEX_LOG_INFO("   "
                          << "Name : " << properties.deviceName);
            std::string device_type;
            switch (properties.deviceType) {
                default: device_type = "UNKNOWN"; break;
                case VK_PHYSICAL_DEVICE_TYPE_OTHER: device_type = "OTHER"; break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "INTEGRATED_GPU "; break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "DISCRETE_GPU"; break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: device_type = "VIRTUAL_GPU"; break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU"; break;
            }
            VKEX_LOG_INFO("   "
                          << "Type : " << device_type);
        }
        // AMDD shader core properties
        if (m_create_info.physical_device->IsAMD()) {
            VkPhysicalDeviceShaderCorePropertiesAMD shader_core_properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD};

            VkPhysicalDeviceProperties2 properties_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            properties_2.pNext                       = &shader_core_properties;

            vkGetPhysicalDeviceProperties2(*m_create_info.physical_device, &properties_2);

            uint32_t cu_count = shader_core_properties.shaderEngineCount *
                                shader_core_properties.shaderArraysPerEngineCount *
                                shader_core_properties.computeUnitsPerShaderArray;

            // clang-format off
            VKEX_LOG_INFO("AMD Shader Core Properties");
            VKEX_LOG_INFO("   " << "Compute Unit Count             : " << cu_count);
            VKEX_LOG_INFO("   " << "Shader Engine Count            : " << shader_core_properties.shaderEngineCount);
            VKEX_LOG_INFO("   " << "Shader Arrays Per Engine Count : " << shader_core_properties.shaderArraysPerEngineCount);
            VKEX_LOG_INFO("   " << "Compute Units Per Shader Array : " << shader_core_properties.computeUnitsPerShaderArray);
            VKEX_LOG_INFO("   " << "SIMD Per Compute Unit          : " << shader_core_properties.simdPerComputeUnit);
            VKEX_LOG_INFO("   " << "Wavefronts Per SIMD            : " << shader_core_properties.wavefrontsPerSimd);
            VKEX_LOG_INFO("   " << "Wavefront Size                 : " << shader_core_properties.wavefrontSize);
            VKEX_LOG_INFO("   " << "SGPRs Per SIMD                 : " << shader_core_properties.sgprsPerSimd);
            VKEX_LOG_INFO("   " << "Min SGPR Allocation            : " << shader_core_properties.minSgprAllocation);
            VKEX_LOG_INFO("   " << "Max SGPR Allocation            : " << shader_core_properties.maxSgprAllocation);
            VKEX_LOG_INFO("   " << "SGPR Allocation Granularity    : " << shader_core_properties.sgprAllocationGranularity);
            VKEX_LOG_INFO("   " << "VGPRs Per SIMD                 : " << shader_core_properties.vgprsPerSimd);
            VKEX_LOG_INFO("   " << "Min VGPR Allocation            : " << shader_core_properties.minVgprAllocation);
            VKEX_LOG_INFO("   " << "Max VGPR Allocation            : " << shader_core_properties.maxVgprAllocation);
            VKEX_LOG_INFO("   " << "VGPR Allocation Granularity    : " << shader_core_properties.vgprAllocationGranularity);
            // clang-format on
        }
        // Extensions
        if (m_vk_create_info.enabledExtensionCount > 0) {
            VKEX_LOG_INFO("Device extensions loaded:");
            for (uint32_t i = 0; i < m_vk_create_info.enabledExtensionCount; ++i) {
                VKEX_LOG_INFO("   " << m_vk_create_info.ppEnabledExtensionNames[i]);
            }
        }
        VKEX_LOG_INFO("");
    }

    // Initialize VMA allocator
    {
        VmaAllocatorCreateInfo vma_allocator_create_info = {};
        vma_allocator_create_info.physicalDevice         = *m_create_info.physical_device;
        vma_allocator_create_info.device                 = m_vk_object;
        vma_allocator_create_info.instance               = GetInstance()->GetVkObject();

        if (m_create_info.enabled_features.bufferDeviceAddress.bufferDeviceAddress) {
            vma_allocator_create_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        VkResult vk_result = InvalidValue<VkResult>::Value;
        VKEX_VULKAN_RESULT_CALL(
            vk_result,
            vmaCreateAllocator(
                &vma_allocator_create_info,
                &m_vma_allocator));
    }

    // Initialize queue slots
    {
        vkex::Result vkex_result = InitializeQueues();
        if (!vkex_result) {
            return vkex_result;
        }
    }

    return vkex::Result::Success;
}

#define VKEX_DESTROY_ALL_OBJECTS(OBJ_TYPE, ALLOCATOR, STORAGE)  \
    {                                                           \
        vkex::Result vkex_result = DestroyAllObjects<OBJ_TYPE>( \
            ALLOCATOR, STORAGE);                                \
        if (!vkex_result) {                                     \
            return vkex_result;                                 \
        }                                                       \
    }

vkex::Result CDevice::DestroyAllStoredObjects(const VkAllocationCallbacks* p_allocator)
{
    // Destroy VKEX objects
    VKEX_DESTROY_ALL_OBJECTS(vkex::ShaderProgram, m_stored_shader_programs, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Texture, m_stored_textures, p_allocator);

    // Destroy Vulkan objects
    VKEX_DESTROY_ALL_OBJECTS(vkex::Buffer, m_stored_buffers, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::CommandPool, m_stored_command_pools, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::ComputePipeline, m_stored_compute_pipelines, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::DescriptorPool, m_stored_descriptor_pools, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::DescriptorSetLayout, m_stored_descriptor_set_layouts, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Fence, m_stored_fences, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::GraphicsPipeline, m_stored_graphics_pipelines, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Image, m_stored_images, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::ImageView, m_stored_image_views, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::PipelineCache, m_stored_pipeline_caches, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::PipelineLayout, m_stored_pipeline_layouts, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::QueryPool, m_stored_query_pools, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Queue, m_stored_queues, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Sampler, m_stored_samplers, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Semaphore, m_stored_semaphores, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::ShaderModule, m_stored_shader_modules, p_allocator);
    VKEX_DESTROY_ALL_OBJECTS(vkex::Swapchain, m_stored_swapchains, p_allocator);

    return vkex::Result::Success;
}

#undef VKEX_DESTROY_ALL_OBJECTS

vkex::Result CDevice::InternalDestroy(const VkAllocationCallbacks* p_allocator)
{
    // Wait for device idle
    {
        VkResult vk_result = InvalidValue<VkResult>::Value;
        VKEX_VULKAN_RESULT_CALL(vk_result, this->WaitIdle());
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }
    }

    // Destroy all stored objects
    {
        vkex::Result vkex_result = DestroyAllStoredObjects(p_allocator);
        if (!vkex_result) {
            return vkex_result;
        }
    }

    // Destroy VMA allocator
    {
        vmaDestroyAllocator(m_vma_allocator);
    }

    // Destroy Vulkan device object
    if (m_vk_object != VK_NULL_HANDLE) {
        vkDestroyDevice(
            m_vk_object,
            p_allocator);

        m_vk_object = VK_NULL_HANDLE;
    }

    return vkex::Result::Success;
}

bool CDevice::IsDebugEnabled() const
{
    return m_instance->IsDebugEnabled();
}

const VkPhysicalDeviceDescriptorBufferPropertiesEXT& CDevice::GetDescriptorBufferProperties() const
{
    return GetPhysicalDevice()->GetPhysicalDeviceProperties().ext.descriptorBuffer;
}

vkex::Result CDevice::GetQueue(
    VkQueueFlagBits queue_type,
    uint32_t        queue_family_index,
    uint32_t        queue_index,
    vkex::Queue*    p_queue) const
{
    auto it = FindIf(
        m_stored_queues,
        [queue_type, queue_family_index, queue_index](const std::unique_ptr<CQueue>& elem) -> bool {
            // There should never be a null IQueue object
            VKEX_ASSERT_MSG(elem, "Null IQueue object encountered!");
            auto&    elem_supported_queue_flags = elem->GetSupportedQueueFlags();
            uint32_t elem_queue_family_index    = elem->GetVkQueueFamilyIndex();
            uint32_t elem_queue_index           = elem->GetVkQueueIndex();
            bool     has_queue_type             = (elem_supported_queue_flags.flags & queue_type);
            bool     has_queue_family_index     = (elem_queue_family_index == queue_family_index);
            bool     has_queue_index            = (elem_queue_index == queue_index);
            bool     found                      = has_queue_type && has_queue_family_index && has_queue_index;
            return found;
        });

    bool found = (it != std::end(m_stored_queues));
    if (!found) {
        return vkex::Result::ErrorSupportedQueueSlotNotFound;
    }

    *p_queue = (*it).get();

    return vkex::Result::Success;
}

VkResult CDevice::WaitIdle()
{
    VkResult vk_result = vkDeviceWaitIdle(m_vk_object);
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }
    return VK_SUCCESS;
}

vkex::Result CDevice::CreateBuffer(
    const vkex::BufferCreateInfo& create_info,
    vkex::Buffer*                 p_object,
    const VkAllocationCallbacks*  p_allocator)
{
    vkex::Result vkex_result = CreateObject<CBuffer>(
        create_info,
        p_allocator,
        m_stored_buffers,
        &CBuffer::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyBuffer(
    vkex::Buffer                 object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CBuffer>(
        m_stored_buffers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateCommandPool(
    const vkex::CommandPoolCreateInfo& create_info,
    vkex::CommandPool*                 p_object,
    const VkAllocationCallbacks*       p_allocator)
{
    vkex::Result vkex_result = CreateObject<CCommandPool>(
        create_info,
        p_allocator,
        m_stored_command_pools,
        &CCommandPool::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyCommandPool(
    vkex::CommandPool            object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CCommandPool>(
        m_stored_command_pools,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateComputePipeline(
    const vkex::ComputePipelineCreateInfo& create_info,
    vkex::ComputePipeline*                 p_object,
    const VkAllocationCallbacks*           p_allocator)
{
    vkex::Result vkex_result = CreateObject<CComputePipeline>(
        create_info,
        p_allocator,
        m_stored_compute_pipelines,
        &CComputePipeline::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyComputePipeline(
    vkex::ComputePipeline        object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CComputePipeline>(
        m_stored_compute_pipelines,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateConstantBuffer(
    const vkex::BufferCreateInfo& create_info,
    vkex::Buffer*                 p_object,
    const VkAllocationCallbacks*  p_allocator)
{
    // Force usage
    auto use_create_info                            = create_info;
    use_create_info.usage_flags.bits.uniform_buffer = true;

    vkex::Result vkex_result = CreateObject<CBuffer>(
        use_create_info,
        p_allocator,
        m_stored_buffers,
        &CBuffer::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyConstantBuffer(
    vkex::Buffer                 object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CBuffer>(
        m_stored_buffers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateDescriptorSetLayout(
    const vkex::DescriptorSetLayoutCreateInfo& create_info,
    vkex::DescriptorSetLayout*                 p_object,
    const VkAllocationCallbacks*               p_allocator)
{
    vkex::Result vkex_result = CreateObject<CDescriptorSetLayout>(
        create_info,
        p_allocator,
        m_stored_descriptor_set_layouts,
        &CDescriptorSetLayout::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyDescriptorSetLayout(
    vkex::DescriptorSetLayout    object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CDescriptorSetLayout>(
        m_stored_descriptor_set_layouts,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateDescriptorSetLayouts(
    const std::vector<vkex::DescriptorSetLayoutCreateInfo>& create_infos,
    std::vector<vkex::DescriptorSetLayout>*                 p_objects,
    const VkAllocationCallbacks*                            p_allocator)
{
    vkex::Result                           vkex_result = vkex::Result::Success;
    std::vector<vkex::DescriptorSetLayout> layouts;
    for (auto& create_info : create_infos) {
        vkex::DescriptorSetLayout layout = nullptr;
        vkex_result                      = this->CreateDescriptorSetLayout(create_info, &layout, p_allocator);
        if (!vkex_result) {
            break;
        }
        layouts.push_back(layout);
    }

    if (!vkex_result) {
        // Destroy any descriptor sets created so far
        {
            vkex::Result vkex_result_2 = this->DestroyDescriptorSetLayouts(layouts, p_allocator);
            if (!vkex_result_2) {
                return vkex_result_2;
            }
        }
        return vkex_result;
    }

    *p_objects = layouts;

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyDescriptorSetLayouts(
    const std::vector<vkex::DescriptorSetLayout>& objects,
    const VkAllocationCallbacks*                  p_allocator)
{
    // This naive and probably needs better handling
    vkex::Result vkex_result = vkex::Result::Success;
    for (auto& object : objects) {
        vkex_result = this->DestroyDescriptorSetLayout(object, p_allocator);
        if (!vkex_result) {
            break;
        }
    }

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateDescriptorPool(
    const vkex::DescriptorPoolCreateInfo& create_info,
    vkex::DescriptorPool*                 p_object,
    const VkAllocationCallbacks*          p_allocator)
{
    vkex::Result vkex_result = CreateObject<CDescriptorPool>(
        create_info,
        p_allocator,
        m_stored_descriptor_pools,
        &CDescriptorPool::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyDescriptorPool(
    vkex::DescriptorPool         object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CDescriptorPool>(
        m_stored_descriptor_pools,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateFence(
    const vkex::FenceCreateInfo& create_info,
    vkex::Fence*                 p_object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = CreateObject<CFence>(
        create_info,
        p_allocator,
        m_stored_fences,
        &CFence::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyFence(
    vkex::Fence                  object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CFence>(
        m_stored_fences,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateGraphicsPipeline(
    const vkex::GraphicsPipelineCreateInfo& create_info,
    vkex::GraphicsPipeline*                 p_object,
    const VkAllocationCallbacks*            p_allocator)
{
    vkex::Result vkex_result = CreateObject<CGraphicsPipeline>(
        create_info,
        p_allocator,
        m_stored_graphics_pipelines,
        &CGraphicsPipeline::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyGraphicsPipeline(
    vkex::GraphicsPipeline       object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CGraphicsPipeline>(
        m_stored_graphics_pipelines,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateImage(
    const vkex::ImageCreateInfo& create_info,
    vkex::Image*                 p_object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = CreateObject<CImage>(
        create_info,
        p_allocator,
        m_stored_images,
        &CImage::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyImage(
    vkex::Image                  object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CImage>(
        m_stored_images,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateImageView(
    const vkex::ImageViewCreateInfo& create_info,
    vkex::ImageView*                 p_object,
    const VkAllocationCallbacks*     p_allocator)
{
    vkex::Result vkex_result = CreateObject<CImageView>(
        create_info,
        p_allocator,
        m_stored_image_views,
        &CImageView::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyImageView(
    vkex::ImageView              object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CImageView>(
        m_stored_image_views,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateIndexBuffer(
    const vkex::BufferCreateInfo& create_info,
    vkex::Buffer*                 p_object,
    const VkAllocationCallbacks*  p_allocator)
{
    // Force usage
    auto use_create_info                          = create_info;
    use_create_info.usage_flags.bits.index_buffer = true;

    vkex::Result vkex_result = CreateObject<CBuffer>(
        use_create_info,
        p_allocator,
        m_stored_buffers,
        &CBuffer::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyIndexBuffer(
    vkex::Buffer                 object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CBuffer>(
        m_stored_buffers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateIndirectBuffer(
    const vkex::BufferCreateInfo& create_info,
    vkex::Buffer*                 p_object,
    const VkAllocationCallbacks*  p_allocator)
{
    // Force usage
    auto use_create_info                             = create_info;
    use_create_info.usage_flags.bits.indirect_buffer = true;

    vkex::Result vkex_result = CreateObject<CBuffer>(
        use_create_info,
        p_allocator,
        m_stored_buffers,
        &CBuffer::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyIndirectBuffer(
    vkex::Buffer                 object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CBuffer>(
        m_stored_buffers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreatePipelineCache(
    const vkex::PipelineCacheCreateInfo& create_info,
    vkex::PipelineCache*                 p_object,
    const VkAllocationCallbacks*         p_allocator)
{
    vkex::Result vkex_result = CreateObject<CPipelineCache>(
        create_info,
        p_allocator,
        m_stored_pipeline_caches,
        &CPipelineCache::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyPipelineCache(
    vkex::PipelineCache          object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CPipelineCache>(
        m_stored_pipeline_caches,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreatePipelineLayout(
    const vkex::PipelineLayoutCreateInfo& create_info,
    vkex::PipelineLayout*                 p_object,
    const VkAllocationCallbacks*          p_allocator)
{
    vkex::Result vkex_result = CreateObject<CPipelineLayout>(
        create_info,
        p_allocator,
        m_stored_pipeline_layouts,
        &CPipelineLayout::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyPipelineLayout(
    vkex::PipelineLayout         object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CPipelineLayout>(
        m_stored_pipeline_layouts,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateQueryPool(
    const vkex::QueryPoolCreateInfo& create_info,
    vkex::QueryPool*                 p_object,
    const VkAllocationCallbacks*     p_allocator)
{
    vkex::Result vkex_result = CreateObject<CQueryPool>(
        create_info,
        p_allocator,
        m_stored_query_pools,
        &CQueryPool::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyQueryPool(
    vkex::QueryPool              object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CQueryPool>(
        m_stored_query_pools,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateSampler(
    const vkex::SamplerCreateInfo& create_info,
    vkex::Sampler*                 p_object,
    const VkAllocationCallbacks*   p_allocator)
{
    vkex::Result vkex_result = CreateObject<CSampler>(
        create_info,
        p_allocator,
        m_stored_samplers,
        &CSampler::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroySampler(
    vkex::Sampler                object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CSampler>(
        m_stored_samplers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateSemaphore(
    const vkex::SemaphoreCreateInfo& create_info,
    vkex::Semaphore*                 p_object,
    const VkAllocationCallbacks*     p_allocator)
{
    vkex::Result vkex_result = CreateObject<CSemaphore>(
        create_info,
        p_allocator,
        m_stored_semaphores,
        &CSemaphore::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroySemaphore(
    vkex::Semaphore              object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CSemaphore>(
        m_stored_semaphores,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateShaderModule(
    const vkex::ShaderModuleCreateInfo& create_info,
    vkex::ShaderModule*                 p_object,
    const VkAllocationCallbacks*        p_allocator)
{
    vkex::Result vkex_result = CreateObject<CShaderModule>(
        create_info,
        p_allocator,
        m_stored_shader_modules,
        &CShaderModule::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyShaderModule(
    vkex::ShaderModule           object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CShaderModule>(
        m_stored_shader_modules,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateShaderProgram(
    const vkex::ShaderProgramCreateInfo& create_info,
    vkex::ShaderProgram*                 p_object,
    const VkAllocationCallbacks*         p_allocator)
{
    vkex::Result vkex_result = CreateObject<CShaderProgram>(
        create_info,
        p_allocator,
        m_stored_shader_programs,
        &CShaderProgram::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyShaderProgram(
    vkex::ShaderProgram          object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CShaderProgram>(
        m_stored_shader_programs,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateStorageBuffer(
    const vkex::BufferCreateInfo& create_info,
    vkex::Buffer*                 p_object,
    const VkAllocationCallbacks*  p_allocator)
{
    // Force usage
    auto use_create_info                            = create_info;
    use_create_info.usage_flags.bits.storage_buffer = true;

    vkex::Result vkex_result = CreateObject<CBuffer>(
        use_create_info,
        p_allocator,
        m_stored_buffers,
        &CBuffer::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyStorageBuffer(
    vkex::Buffer                 object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CBuffer>(
        m_stored_buffers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateSwapchain(
    const vkex::SwapchainCreateInfo& create_info,
    vkex::Swapchain*                 p_object,
    const VkAllocationCallbacks*     p_allocator)
{
    vkex::Result vkex_result = CreateObject<CSwapchain>(
        create_info,
        p_allocator,
        m_stored_swapchains,
        &CSwapchain::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroySwapchain(
    vkex::Swapchain              object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CSwapchain>(
        m_stored_swapchains,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateTexture(
    const vkex::TextureCreateInfo& create_info,
    vkex::Texture*                 p_object,
    const VkAllocationCallbacks*   p_allocator)
{
    vkex::Result vkex_result = CreateObject<CTexture>(
        create_info,
        p_allocator,
        m_stored_textures,
        &CTexture::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyTexture(
    vkex::Texture                object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CTexture>(
        m_stored_textures,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::CreateVertexBuffer(
    const vkex::BufferCreateInfo& create_info,
    vkex::Buffer*                 p_object,
    const VkAllocationCallbacks*  p_allocator)
{
    // Force usage
    auto use_create_info                           = create_info;
    use_create_info.usage_flags.bits.vertex_buffer = true;

    vkex::Result vkex_result = CreateObject<CBuffer>(
        use_create_info,
        p_allocator,
        m_stored_buffers,
        &CBuffer::SetDevice,
        this,
        p_object);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

vkex::Result CDevice::DestroyVertexBuffer(
    vkex::Buffer                 object,
    const VkAllocationCallbacks* p_allocator)
{
    vkex::Result vkex_result = DestroyObject<CBuffer>(
        m_stored_buffers,
        object,
        p_allocator);

    if (!vkex_result) {
        return vkex_result;
    }

    return vkex::Result::Success;
}

} // namespace vkex