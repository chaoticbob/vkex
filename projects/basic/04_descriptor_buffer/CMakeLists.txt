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

project(04_descriptor_buffer)

set(PROJECTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND HDR_FILES
  ${PROJECTS_DIR}/common/AssetUtil.h
)

list(APPEND SRC_FILES
  ${SRC_DIR}/main.cpp
  ${PROJECTS_DIR}/common/AssetUtil.cpp
)

add_executable(${PROJECT_NAME} ${HDR_FILES} ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
  FOLDER "vkex/projects_basic"
)

target_include_directories(${PROJECT_NAME} 
  PRIVATE ${TOP_INC_DIR}
          ${PROJECTS_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE libvkex)