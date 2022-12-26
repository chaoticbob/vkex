#
# Copyright 2018-2023 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(libvkex)

# Determine Vulkan's include directory
if (NOT VULKAN_INCLUDE_DIR)
  if (LINUX)
    if (DEFINED ENV{VULKAN_SDK})
    	set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/x86_64/include")
    endif()
  elseif (WIN32)
    if (DEFINED ENV{VULKAN_SDK})
      set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
    endif()
  endif()
endif()

# Determine Vulkan's library directory
if (NOT VULKAN_LIBRARY_DIR)
  if (LINUX)
    if (DEFINED ENV{VULKAN_SDK})
    	set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/x86_64/lib")
    endif()
  elseif (WIN32)
    if (DEFINED ENV{VULKAN_SDK})
      set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/Lib")
    endif()
  endif()
endif()

# Bail if Vulkan's include directory is not set
if (NOT VULKAN_INCLUDE_DIR)
  message(FATAL_ERROR "VULKAN_INCLUDE_DIR not specified and could not be determined using environment variable VULKAN_SDK")
endif()

# Bail if Vulkan's library directory is not set
if (NOT VULKAN_LIBRARY_DIR)
  message(FATAL_ERROR "VULKAN_LIBRARY_DIR not specified and could not be determined using environment variable VULKAN_SDK")
endif()

message("VKEX is using Vulkan found here: ${VULKAN_DIR}")
message("  Vulkan include directory: ${VULKAN_INCLUDE_DIR}")
message("  Vulkan library directory: ${VULKAN_LIBRARY_DIR}")

set(INC_DIR     ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR})
set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders)

set(GLM_INC_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/glm)
set(STB_INC_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/stb)
set(VMA_INC_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/VulkanMemoryAllocator/src)
set(SPIRV_REFLECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/SPIRV-Reflect)
set(IMGUI_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui)
set(TINY_OBJ_INC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/tinyobjloader)

list (APPEND SHARED_SHADER_HDR_FILES
  ${SHADERS_DIR}/Common.h
)

list(APPEND IMGUI_HDR_FILES
  ${IMGUI_DIR}/imgui.h
  ${IMGUI_DIR}/backends/imgui_impl_glfw.h
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.h
)

list(APPEND IMGUI_SRC_FILES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

list(APPEND VKEX_HDR_FILES
  ${INC_DIR}/vkex.h
  ${INC_DIR}/Application.h
  ${INC_DIR}/ArgParser.h
  ${INC_DIR}/Bitmap.h
  ${INC_DIR}/Buffer.h
  ${INC_DIR}/Camera.h
  ${INC_DIR}/Cast.h
  ${INC_DIR}/Command.h
  ${INC_DIR}/Config.h
  ${INC_DIR}/CpuResource.h
  ${INC_DIR}/Descriptor.h
  ${INC_DIR}/Device.h
  ${INC_DIR}/Entity.h
  ${INC_DIR}/FileSystem.h
  ${INC_DIR}/Forward.h
  ${INC_DIR}/Geometry.h
  ${INC_DIR}/Image.h
  ${INC_DIR}/Instance.h
  ${INC_DIR}/Log.h
  ${INC_DIR}/Pipeline.h
  ${INC_DIR}/QueryPool.h
  ${INC_DIR}/Queue.h
  #${INC_DIR}/RenderPass.h
  ${INC_DIR}/Sampler.h
  ${INC_DIR}/Shader.h
  ${INC_DIR}/Swapchain.h
  ${INC_DIR}/Sync.h
  ${INC_DIR}/Texture.h
  ${INC_DIR}/Timer.h
  ${INC_DIR}/ToString.h
  ${INC_DIR}/Traits.h
  ${INC_DIR}/Transform.h
  ${INC_DIR}/Util.h
  #${INC_DIR}/View.h
  #${INC_DIR}/VkexLoader.h
  #${INC_DIR}/VkexLoaderHelper.h
  ${INC_DIR}/VulkanUtil.h
  ${SPIRV_REFLECT_DIR}/spirv_reflect.h
  ${SHARED_SHADER_HDR_FILES}
)

list(APPEND VKEX_SRC_FILES
  ${SRC_DIR}/Application.cpp
  ${SRC_DIR}/ArgParser.cpp
  ${SRC_DIR}/Bitmap.cpp
  ${SRC_DIR}/Buffer.cpp
  ${SRC_DIR}/Camera.cpp
  ${SRC_DIR}/Cast.cpp
  ${SRC_DIR}/Command.cpp
  ${SRC_DIR}/CpuResource.cpp
  ${SRC_DIR}/Descriptor.cpp
  ${SRC_DIR}/Device.cpp
  ${SRC_DIR}/Entity.cpp
  ${SRC_DIR}/Geometry.cpp
  ${SRC_DIR}/Image.cpp
  ${SRC_DIR}/Instance.cpp
  ${SRC_DIR}/Log.cpp
  ${SRC_DIR}/Pipeline.cpp
  ${SRC_DIR}/QueryPool.cpp
  ${SRC_DIR}/Queue.cpp
  #${SRC_DIR}/RenderPass.cpp
  ${SRC_DIR}/Sampler.cpp
  ${SRC_DIR}/Shader.cpp
  ${SRC_DIR}/Swapchain.cpp
  ${SRC_DIR}/Sync.cpp
  ${SRC_DIR}/Texture.cpp
  ${SRC_DIR}/Timer.cpp
  ${SRC_DIR}/ToString.cpp
  ${SRC_DIR}/Transform.cpp
  #${SRC_DIR}/View.cpp
  #${SRC_DIR}/VkexLoader.cpp
  #${SRC_DIR}/VkexLoaderHelper.cpp
  ${SRC_DIR}/VulkanUtil.cpp
  ${SPIRV_REFLECT_DIR}/spirv_reflect.c
)

list(APPEND HDR_FILES
  ${VKEX_HDR_FILES}
  ${IMGUI_HDR_FILES}
)

list(APPEND SRC_FILES
  ${VKEX_SRC_FILES}
  ${SHADER_FILES}
  ${IMGUI_SRC_FILES}
)

add_library(${PROJECT_NAME} STATIC ${HDR_FILES} ${SRC_FILES})

source_group("vkex\\Header Files"          FILES ${VKEX_HDR_FILES})
source_group("vkex\\Header Files\\shaders" FILES ${SHARED_SHADER_HDR_FILES})
source_group("vkex\\Source Files"          FILES ${VKEX_SRC_FILES})
source_group("imgui"                       FILES ${IMGUI_HDR_FILES} ${IMGUI_SRC_FILES})

set_target_properties(${PROJECT_NAME}
  PROPERTIES FOLDER vkex
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME}
  PUBLIC GLFW_INCLUDE_NONE)

# Additional compile definitions
if (LINUX)
  target_compile_definitions(${PROJECT_NAME}
    PUBLIC VK_USE_PLATFORM_XCB_KHR
           VKEX_LINUX
           VKEX_LINUX_XCB
  )  
elseif (WIN32)
  target_compile_definitions(${PROJECT_NAME}
    PUBLIC VK_USE_PLATFORM_WIN32_KHR
           VKEX_WIN32
           _CRT_SECURE_NO_WARNINGS
  )
endif()

# Include directories
target_include_directories(${PROJECT_NAME}
  PRIVATE ${STB_INC_DIR}
  PUBLIC  ${TINY_OBJ_INC_DIR}
          ${VKEX_TOP_INC_DIR}
          ${VULKAN_INCLUDE_DIR}
          ${GLM_INC_DIR}
          ${VMA_INC_DIR}
          ${STB_INC_DIR}
          ${SPIRV_REFLECT_DIR}
          ${IMGUI_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
  PUBLIC glfw
)

# Additional link properties
if (LINUX)
  # Link directories
  target_link_directories(${PROJECT_NAME} 
    PUBLIC ${VULKAN_LIBRARY_DIR}
  )
  # Link properties
  set_target_properties(${PROJECT_NAME}
    PROPERTIES OUTPUT_NAME VKEX
               LINKER_LANGUAGE CXX
  )
  # Link libraries
  target_link_libraries(${PROJECT_NAME} 
    PUBLIC vulkan
           xcb
           X11-xcb
  )
elseif (WIN32)
  target_link_libraries(${PROJECT_NAME} 
    PUBLIC "${VULKAN_LIBRARY_DIR}/vulkan-1.lib"
  )
endif()
