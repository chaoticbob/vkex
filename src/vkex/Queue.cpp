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

#include <vkex/Queue.h>
#include <vkex/Device.h>

namespace vkex {

// =================================================================================================
// SubmtInfo
// =================================================================================================
SubmitInfo::SubmitInfo()
{
}

SubmitInfo::~SubmitInfo()
{
}

void SubmitInfo::AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags wait_dst_stage_mask, uint64_t value)
{
  m_wait_semaphores.push_back(semaphore);
  m_wait_dst_stage_masks.push_back(wait_dst_stage_mask);
  m_wait_values.push_back(value);
}

void SubmitInfo::AddWaitSemaphore(const vkex::Semaphore& semaphore, uint64_t value)
{
  AddWaitSemaphore(semaphore->GetVkObject(), semaphore->GetWaitDstStageMask(), value);
}

void SubmitInfo::AddCommandBuffer(VkCommandBuffer command_buffer)
{
  m_command_buffers.push_back(command_buffer);
}

void SubmitInfo::AddCommandBuffer(const vkex::CommandBuffer& command_buffer)
{
  AddCommandBuffer(command_buffer->GetVkObject());
}

void SubmitInfo::AddSignalSemaphore(VkSemaphore semaphore, uint64_t value)
{
  m_signal_semaphores.push_back(semaphore);
  m_signal_values.push_back(value);
}

void SubmitInfo::AddSignalSemaphore(const vkex::Semaphore& semaphore, uint64_t value)
{
  AddSignalSemaphore(semaphore->GetVkObject(), value);
}

void SubmitInfo::SetFence(VkFence fence)
{
  m_fence = fence;
}

void SubmitInfo::SetFence(const vkex::Fence& fence)
{
  SetFence(fence->GetVkObject());
}

// =================================================================================================
// Queue
// =================================================================================================
CQueue::CQueue()
{
}

CQueue::~CQueue()
{
}

vkex::Result CQueue::InternalCreate(
  const vkex::QueueCreateInfo&  create_info,
  const VkAllocationCallbacks*  p_allocator
)
{
  // Copy create info
  m_create_info = create_info;

  return vkex::Result::Success;
}

vkex::Result CQueue::InternalDestroy(const VkAllocationCallbacks* p_allocator)
{
  return vkex::Result::Success;
}

VkBool32 CQueue::SupportsPresent(const vkex::DisplayInfo& display_info) const
{
  VkBool32 supported = GetDevice()->GetPhysicalDevice()->SupportsPresent(
    m_create_info.queue_family_index,
    display_info);
  return supported;
}

VkResult CQueue::WaitIdle()
{
  VkResult vk_result = vkex::QueueWaitIdle(
    m_create_info.vk_object);
  if (vk_result != VK_SUCCESS) {
    return vk_result;
  }
  return VK_SUCCESS;
}

vkex::Result CQueue::Submit(const vkex::SubmitInfo& submit_info)
{
  const std::vector<VkSemaphore>&          vk_wait_semaphores       = submit_info.GetWaitSemaphores();
  const std::vector<VkPipelineStageFlags>& vk_wait_dst_stage_masks  = submit_info.GetWaitDstStageMasks();
  const std::vector<VkCommandBuffer>&      vk_command_buffers       = submit_info.GetCommandBuffers();
  const std::vector<VkSemaphore>&          vk_signal_semaphores     = submit_info.GetSignalSemaphores();
  VkFence                                  vk_fence                 = submit_info.GetFence();
  const std::vector<uint64_t>&             wait_values              = submit_info.GetWaitValues();
  const std::vector<uint64_t>&             signal_values            = submit_info.GetSignalValues();

  VkSubmitInfo vk_submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
  vk_submit_info.waitSemaphoreCount    = vkex::CountU32(vk_wait_semaphores);
  vk_submit_info.pWaitSemaphores       = vkex::DataPtr(vk_wait_semaphores);
  vk_submit_info.pWaitDstStageMask     = vkex::DataPtr(vk_wait_dst_stage_masks);
  vk_submit_info.commandBufferCount    = vkex::CountU32(vk_command_buffers);
  vk_submit_info.pCommandBuffers       = vkex::DataPtr(vk_command_buffers);
  vk_submit_info.signalSemaphoreCount  = vkex::CountU32(vk_signal_semaphores);
  vk_submit_info.pSignalSemaphores     = vkex::DataPtr(vk_signal_semaphores);

#if defined(VKEX_ENABLE_TIMELINE_SEMAPHORE)
  VkTimelineSemaphoreSubmitInfoKHR vk_timeline_semaphore_submit_info = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR };
  if (GetDevice()->GetEnabledFeatures().timelineSemaphore) {
    vk_timeline_semaphore_submit_info.waitSemaphoreValueCount   = vkex::CountU32(wait_values);
    vk_timeline_semaphore_submit_info.pWaitSemaphoreValues      = vkex::DataPtr(wait_values);
    vk_timeline_semaphore_submit_info.signalSemaphoreValueCount = vkex::CountU32(signal_values);
    vk_timeline_semaphore_submit_info.pSignalSemaphoreValues    = vkex::DataPtr(signal_values);
    // Add to pNext
    vk_submit_info.pNext = &vk_timeline_semaphore_submit_info;
  }
#endif

  VkResult vk_result = vkex::QueueSubmit(m_create_info.vk_object, 1, &vk_submit_info, vk_fence);
  if (vk_result != VK_SUCCESS) {
    return vkex::Result(vk_result);
  }

  return vkex::Result::Success;
}

} // namespace vkex