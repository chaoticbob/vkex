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

#include "AssetUtil.h"

namespace asset_util {

std::vector<uint8_t> LoadFile(const vkex::fs::path& file_path)
{
  if (!vkex::fs::exists(file_path) || !vkex::fs::is_regular_file(file_path)) {
    VKEX_LOG_ERROR("File does not exist: " << file_path);
    return std::vector<uint8_t>();
  }

  std::vector<uint8_t> data = vkex::fs::load_file(file_path);
  VKEX_LOG_INFO("File loaded: " << file_path);

  return data;
}

vkex::Result CreateTexture(
  const vkex::fs::path& image_file_path,
  vkex::Queue           queue,
  bool                  host_visible,
  vkex::Texture*        p_texture)
{
  VKEX_ASSERT_MSG(queue != nullptr, "Queue is null");
  VKEX_ASSERT_MSG(p_texture != nullptr, "Target texture object is null");

  vkex::Device device = queue->GetDevice();

  // Load file data
  auto file_data = LoadFile(image_file_path);
  VKEX_ASSERT_MSG(!file_data.empty(), "Texture failed to load!");

  // Load bitmap
  std::unique_ptr<vkex::Bitmap> bitmap;
  VKEX_CALL(vkex::Bitmap::Create(
    file_data.size(),
    file_data.data(),
    0,
    &bitmap));

  // Create staging buffer and copy bitmap
  vkex::Buffer cpu_buffer = nullptr;
  {
    uint64_t data_size = bitmap->GetDataSizeAllLevels();

    vkex::BufferCreateInfo create_info        = {};
    create_info.size                          = data_size;
    create_info.usage_flags.bits.transfer_src = true;
    create_info.committed                     = true;
    create_info.memory_usage                  = VMA_MEMORY_USAGE_CPU_TO_GPU;
    VKEX_CALL(device->CreateStorageBuffer(create_info, &cpu_buffer));

    void* address = nullptr;
    VkResult vk_result = cpu_buffer->MapMemory(&address);
    VKEX_ASSERT(vk_result == VK_SUCCESS);
    memcpy(address, bitmap->GetData(), data_size);
    cpu_buffer->UnmapMemory();
  }

 
  // Create image
  {
    vkex::TextureCreateInfo create_info             = {};
    create_info.image.image_type                    = VK_IMAGE_TYPE_2D;
    create_info.image.format                        = bitmap->GetFormat();
    create_info.image.extent                        = bitmap->GetExtent();
    create_info.image.mip_levels                    = bitmap->GetMipLevels();
    create_info.image.tiling                        = VK_IMAGE_TILING_OPTIMAL;
    create_info.image.usage_flags.bits.transfer_dst = true;
    create_info.image.initial_layout                = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.image.committed                     = true;
    create_info.image.memory_usage                  = (host_visible ? VMA_MEMORY_USAGE_CPU_ONLY : VMA_MEMORY_USAGE_GPU_ONLY);
    create_info.view.derive_from_image              = true;
    VKEX_CALL(device->CreateTexture(create_info, p_texture));
  }

  // Transition from VK_IMAGE_LAYOUT_PREINITIALIZED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  // for VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT.
  VKEX_CALL(vkex::TransitionImageLayout(
    queue,
    (*p_texture)->GetImage(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));

  std::vector<VkBufferImageCopy> regions;
  for (uint32_t level = 0; level < bitmap->GetMipLevels(); ++level) {
    vkex::Bitmap::Mip mip = {};
    bitmap->GetMipLayout(level, &mip);
    VkBufferImageCopy region               = {};
    region.bufferOffset                    = mip.data_offset;
    region.bufferRowLength                 = mip.width;
    region.bufferImageHeight               = mip.height;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = level;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset.x                   = 0;
    region.imageOffset.y                   = 0;
    region.imageOffset.z                   = 0;
    region.imageExtent.width               = mip.width;
    region.imageExtent.height              = mip.height;
    region.imageExtent.depth               = 1;
    regions.push_back(region);
  }

  vkex::CopyResource(
    queue,
    cpu_buffer,
    (*p_texture)->GetImage(),
    vkex::CountU32(regions),
    vkex::DataPtr(regions));

  // Transition from VK_IMAGE_LAYOUT_PREINITIALIZED to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  // for VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT.
  VKEX_CALL(vkex::TransitionImageLayout(
    queue,
    (*p_texture)->GetImage(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)); 

  // Destroy temp CPU buffer
  VKEX_CALL(device->DestroyStorageBuffer(cpu_buffer));

  return vkex::Result::Success;
}

} // namespace asset_util
