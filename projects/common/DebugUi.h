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

#ifndef __COMMON_DEBUG_UI_H__
#define __COMMON_DEBUG_UI_H__

#include "vkex/Application.h"

void AddRow(const std::string& label, float value);

void DrawDebugUiPhyiscalDevice(vkex::PhysicalDevice physical_device);

#endif // __COMMON_DEBUG_UI_H__