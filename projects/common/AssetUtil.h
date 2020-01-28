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

#ifndef __COMMON_ASSET_UTIL_H__
#define __COMMON_ASSET_UTIL_H__

#include "vkex/Application.h"

namespace asset_util {

std::vector<uint8_t> LoadFile(const vkex::fs::path& file_path);

vkex::Result CreateTexture(
  const vkex::fs::path& image_file_path,
  vkex::Queue           queue,
  bool                  host_visible,
  vkex::Texture*        p_texture);

} // namespace asset_util

#endif // __COMMON_ASSET_UTIL_H__