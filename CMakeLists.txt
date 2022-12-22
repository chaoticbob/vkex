#
# Copyright 2018-2019 Google Inc.
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

# Force these policies to NEW since OLD is deprecated
cmake_policy(SET CMP0054 NEW)

project(vkex)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
  message("Using CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

# Detect GGP
if (GGP_TOOLCHAIN_PATH)
	set(GGP TRUE)
endif()

# Detect Linux
if(UNIX AND NOT APPLE)
		set(LINUX TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

if(NOT DEFINED VKEX_BIN_DIR)
    set(VKEX_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
endif()

if(NOT DEFINED VKEX_LIB_DIR)
    set(VKEX_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${VKEX_BIN_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${VKEX_LIB_DIR})

set(VKEX_TOP_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     FALSE CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC  FALSE CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

add_subdirectory(src)
add_subdirectory(projects)
