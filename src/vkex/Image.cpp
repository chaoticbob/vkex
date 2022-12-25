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
#include "vkex/Image.h"
#include "vkex/ToString.h"

namespace vkex {

// =================================================================================================
// Image
// =================================================================================================
CImage::CImage()
{
}

CImage::~CImage()
{
}

VkResult CImage::InitializeCommitted()
{
    VkResult vk_result = AllocateMemory();
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }
    // Bind
    vk_result = BindMemory();
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }
    // Map
    if ((m_create_info.memory_usage == VMA_MEMORY_USAGE_CPU_TO_GPU) || (m_create_info.memory_usage == VMA_MEMORY_USAGE_GPU_TO_CPU)) {
        void* p_mapped_address = nullptr;
        vk_result              = MapMemory(&p_mapped_address);
        if (vk_result != VK_SUCCESS) {
            return vk_result;
        }
    }
    return VK_SUCCESS;
}

vkex::Result CImage::InternalCreate(
    const vkex::ImageCreateInfo& create_info,
    const VkAllocationCallbacks* p_allocator)
{
    // Copy create info
    m_create_info = create_info;

    // Fill Vulkan create info
    {
        m_vk_create_info                       = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        m_vk_create_info.flags                 = m_create_info.create_flags;
        m_vk_create_info.imageType             = m_create_info.image_type;
        m_vk_create_info.format                = m_create_info.format;
        m_vk_create_info.extent                = m_create_info.extent;
        m_vk_create_info.mipLevels             = m_create_info.mip_levels;
        m_vk_create_info.arrayLayers           = m_create_info.array_layers;
        m_vk_create_info.samples               = m_create_info.samples;
        m_vk_create_info.tiling                = m_create_info.tiling;
        m_vk_create_info.usage                 = m_create_info.usage_flags.flags;
        m_vk_create_info.sharingMode           = m_create_info.sharing_mode;
        m_vk_create_info.queueFamilyIndexCount = CountU32(m_create_info.queue_family_indices);
        m_vk_create_info.pQueueFamilyIndices   = DataPtr(m_create_info.queue_family_indices);
        m_vk_create_info.initialLayout         = m_create_info.initial_layout;
    }

    if (m_create_info.vk_object) {
        // Copy Vulkan object
        m_vk_object = m_create_info.vk_object;
    }
    else {
        VkResult vk_result = InvalidValue<VkResult>::Value;
        VKEX_VULKAN_RESULT_CALL(
            vk_result, vkCreateImage(*m_device, &m_vk_create_info, p_allocator, &m_vk_object));
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }
    }

    // Allocate and bind if resource is committed.
    if (m_create_info.committed) {
        VkResult vk_result = InvalidValue<VkResult>::Value;
        VKEX_VULKAN_RESULT_CALL(vk_result, InitializeCommitted());
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }
    }

    // Image aspect
    m_aspect_flags = vkex::DetermineAspectMask(m_create_info.format);

    return vkex::Result::Success;
}

vkex::Result CImage::InternalDestroy(const VkAllocationCallbacks* p_allocator)
{
    // Free memory
    FreeMemory();

    if (m_vk_object != VK_NULL_HANDLE) {
        if (m_create_info.vk_object != VK_NULL_HANDLE) {
            m_create_info.vk_object = VK_NULL_HANDLE;
        }
        else {
            vkDestroyImage(*m_device, m_vk_object, p_allocator);
        }

        m_vk_object = VK_NULL_HANDLE;
    }

    return vkex::Result::Success;
}

VkResult CImage::AllocateMemory()
{
    m_vma_allocation_create_info                = {};
    m_vma_allocation_create_info.flags          = 0;
    m_vma_allocation_create_info.usage          = m_create_info.memory_usage;
    m_vma_allocation_create_info.requiredFlags  = 0;
    m_vma_allocation_create_info.preferredFlags = 0;
    m_vma_allocation_create_info.memoryTypeBits = 0;
    m_vma_allocation_create_info.pool           = m_create_info.memory_pool;
    m_vma_allocation_create_info.pUserData      = nullptr;

    VkResult vk_result = vmaAllocateMemoryForImage(
        m_device->GetVmaAllocator(),
        m_vk_object,
        &m_vma_allocation_create_info,
        &m_vma_allocation,
        &m_vma_allocation_info);
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }

    return VK_SUCCESS;
}

void CImage::FreeMemory()
{
    if (m_vma_allocation != VK_NULL_HANDLE) {
        if (IsMemoryMapped()) {
            UnmapMemory();
        }

        vmaFreeMemory(m_device->GetVmaAllocator(), m_vma_allocation);

        m_vma_allocation = VK_NULL_HANDLE;
    }
}

bool CImage::IsMemoryAllocated() const
{
    bool is_allocated = (m_vma_allocation != VK_NULL_HANDLE);
    return is_allocated;
}

VkResult CImage::BindMemory()
{
    VkResult vk_result =
        vmaBindImageMemory(m_device->GetVmaAllocator(), m_vma_allocation, m_vk_object);
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }

    return VK_SUCCESS;
}

VkResult CImage::MapMemory(void** pp_mapped_address)
{
    bool has_allocation = (m_vma_allocation != VK_NULL_HANDLE);
    VKEX_ASSERT_MSG(
        (m_vma_allocation != VK_NULL_HANDLE),
        "Attempting to map image that doesn't have an allocation!");
    if (m_vma_allocation == VK_NULL_HANDLE) {
        // Not the most ideal error message
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (m_mapped_address != nullptr) {
        *pp_mapped_address = m_mapped_address;
        return VK_SUCCESS;
    }

    VkResult vk_result =
        vmaMapMemory(m_device->GetVmaAllocator(), m_vma_allocation, &m_mapped_address);
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }

    *pp_mapped_address = m_mapped_address;

    return VK_SUCCESS;
}

void CImage::UnmapMemory()
{
    vmaUnmapMemory(m_device->GetVmaAllocator(), m_vma_allocation);

    m_mapped_address = nullptr;
}

bool CImage::IsMemoryMapped() const
{
    bool is_mapped = (m_mapped_address != nullptr);
    return is_mapped;
}

VkDeviceSize CImage::GetMemoryOffset() const
{
    return m_vma_allocation_info.offset;
}

VkDeviceSize CImage::GetMemorySize() const
{
    return m_vma_allocation_info.size;
}

VmaAllocationInfo CImage::GetVmaAllocationInfo() const
{
    return m_vma_allocation_info;
}

VkImageSubresourceLayers CImage::GetSubresourceLayers(
    uint32_t mip_level,
    uint32_t array_layer_start,
    uint32_t array_layer_count) const
{
    VkImageSubresourceLayers subresource_layer = {};
    subresource_layer.aspectMask               = GetAspectFlags();
    subresource_layer.mipLevel                 = mip_level;
    subresource_layer.baseArrayLayer           = array_layer_start;
    subresource_layer.layerCount               = array_layer_count;
    return subresource_layer;
}

// =================================================================================================
// ImageView
// =================================================================================================
CImageView::CImageView()
{
}

CImageView::~CImageView()
{
}

vkex::Result CImageView::InternalCreate(
    const vkex::ImageViewCreateInfo& create_info,
    const VkAllocationCallbacks*     p_allocator)
{
    // Copy create info
    m_create_info = create_info;

    // Vulkan create info
    {
        // Image
        VkImage vk_image = m_create_info.image->GetVkObject();
        // Create info
        m_vk_create_info                  = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        m_vk_create_info.flags            = m_create_info.create_flags;
        m_vk_create_info.image            = vk_image;
        m_vk_create_info.viewType         = m_create_info.view_type;
        m_vk_create_info.format           = m_create_info.format;
        m_vk_create_info.components       = m_create_info.components;
        m_vk_create_info.subresourceRange = m_create_info.subresource_range;
    }

    // Create image view
    {
        VkResult vk_result = InvalidValue<VkResult>::Value;
        VKEX_VULKAN_RESULT_CALL(
            vk_result, vkCreateImageView(*m_device, &m_vk_create_info, p_allocator, &m_vk_object));
        if (vk_result != VK_SUCCESS) {
            return vkex::Result(vk_result);
        }
    }

    return vkex::Result::Success;
}

vkex::Result CImageView::InternalDestroy(const VkAllocationCallbacks* p_allocator)
{
    if (m_vk_object != VK_NULL_HANDLE) {
        vkDestroyImageView(*m_device, m_vk_object, p_allocator);

        m_vk_object = VK_NULL_HANDLE;
    }
    return vkex::Result::Success;
}

} // namespace vkex