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

project(vkex)

# Detect GGP
if (GGP_TOOLCHAIN_PATH)
	set(GGP TRUE)
endif()

# Detect Linux
if(UNIX AND NOT APPLE)
		set(LINUX TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(VKEX_TOP_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     FALSE CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC  FALSE CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

add_subdirectory(src)
add_subdirectory(projects)
