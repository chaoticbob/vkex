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

#ifndef __VKEX_TO_STRING_H__
#define __VKEX_TO_STRING_H__

#include "vkex/Config.h"

#include <iomanip>
#include <type_traits>

namespace vkex {

struct TextFormat
{
    std::string block_indent            = "";
    bool        skip_first_block_indent = false;
    std::string array_element_indent    = "";
    std::string array_struct_indent     = "";
};

std::string ToString(uint32_t value, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(float value, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const char* value, const vkex::TextFormat& format = vkex::TextFormat());

std::string ToString(VkResult value);

std::string ToStringShort(VkPhysicalDeviceType value);
std::string ToString(VkPhysicalDeviceType value);

std::string ToStringShort(VkFormat value);
std::string ToString(VkFormat value);

std::string ToStringShort(VkColorSpaceKHR value);
std::string ToString(VkColorSpaceKHR value);

std::string ToStringShort(VkPresentModeKHR value);
std::string ToString(VkPresentModeKHR value);

std::string ToString(VkSharingMode value);
std::string ToString(VkSurfaceTransformFlagBitsKHR value);
std::string ToString(VkCompositeAlphaFlagBitsKHR value);

std::string ToString(const VkExtent2D& value);
std::string ToString(const VkPhysicalDeviceFeatures& value, const vkex::TextFormat& format = vkex::TextFormat());

std::string ToString(const VkApplicationInfo& application_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkInstanceCreateInfo& create_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkDeviceQueueCreateInfo& create_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkDeviceCreateInfo& create_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkSwapchainCreateInfoKHR& create_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkBufferCreateInfo& create_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkImageCreateInfo& create_info, const vkex::TextFormat& format = vkex::TextFormat());
std::string ToString(const VkImageViewCreateInfo& create_info, const vkex::TextFormat& format = vkex::TextFormat());

template <typename T>
std::string ToHexString(T* ptr, bool pad_zero = true, bool upper_case_alpha = true, bool prefix = true)
{
    std::stringstream ss;
    ss << std::setfill('0');
    if (sizeof(ptr) == 8) {
        if (pad_zero) {
            ss << std::setw(16);
        }
        ss << std::hex << reinterpret_cast<uintptr_t>(ptr);
    }
    else if (sizeof(ptr) == 4) {
        if (pad_zero) {
            ss << std::setw(8);
        }
        ss << std::hex << reinterpret_cast<uintptr_t>(ptr);
    }
    std::string s = ss.str();
    if (upper_case_alpha) {
        for (auto& c : s) {
            c = toupper(c);
        }
    }
    if (prefix) {
        s = "0x" + s;
    }
    return s;
}

template <typename T>
std::string ToHexString(T value, bool pad_zero = true, bool upper_case_alpha = true, bool prefix = true)
{
    bool is_integer = std::numeric_limits<T>::is_integer;
    assert(is_integer && "T isn't an integer!");
    std::stringstream ss;
    ss << std::setfill('0');
    if (sizeof(value) == 8) {
        if (pad_zero) {
            ss << std::setw(16);
        }
        ss << std::hex << value;
    }
    else if (sizeof(value) == 4) {
        if (pad_zero) {
            ss << std::setw(8);
        }
        ss << std::hex << value;
    }
    else if (sizeof(value) == 2) {
        if (pad_zero) {
            ss << std::setw(4);
        }
        ss << std::hex << value;
    }
    else if (sizeof(value) == 1) {
        if (pad_zero) {
            ss << std::setw(2);
        }
        ss << std::hex << static_cast<uint32_t>(value);
    }
    std::string s = ss.str();
    if (upper_case_alpha) {
        for (auto& c : s) {
            c = toupper(c);
        }
    }
    if (prefix) {
        s = "0x" + s;
    }
    return s;
}

template <typename T>
std::string ToPointerAddressString(T* ptr, bool pad_zero = true, bool upper_case_alpha = true, bool prefix = true)
{
    return (ptr != nullptr) ? ToHexString(ptr, pad_zero, upper_case_alpha, prefix) : "NULL";
}

template <typename T>
std::string ToVkHandleAddressString(T* ptr, bool pad_zero = true, bool upper_case_alpha = true, bool prefix = true)
{
    return (ptr != VK_NULL_HANDLE) ? ToHexString(ptr, pad_zero, upper_case_alpha, prefix) : "VK_NULL_HANDLE";
}

template <typename T>
std::string ToArrayString(size_t count, const T* p_array, const vkex::TextFormat& format = vkex::TextFormat())
{
    if (count == 0) {
        return "{}";
    }

    std::stringstream ss;
    ss << "{"
       << "\n";
    for (uint32_t i = 0; i < count; ++i) {
        ss << format.block_indent << format.array_element_indent << "[" << i << "]"
           << " = " << vkex::ToString(p_array[i], format) << "\n";
    }
    ss << format.block_indent << "}";
    return ss.str();
}

} // namespace vkex

#endif // __VKEX_TO_STRING_H__