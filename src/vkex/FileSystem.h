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

#ifndef __VKEX_FILE_SYSTEM_H__
#define __VKEX_FILE_SYSTEM_H__

#include <filesystem>
#include <fstream>

namespace vkex::fs {

using namespace std::filesystem;

/*! @fn load_file

   @return Returns array with data loaded from 'p'.
           Returns empty array if load fails.

 */
inline std::vector<uint8_t> load_file(const fs::path& p)
{
    std::vector<uint8_t> data;
    if (exists(p) && fs::is_regular_file(p)) {
        std::ifstream is(p.c_str(), std::ios::binary);
        if (is.is_open()) {
            is.seekg(0, std::ios::end);
            size_t size = is.tellg();
            if (size > 0) {
                data.resize(size);
                is.seekg(0, std::ios::beg);
                is.read(reinterpret_cast<char*>(data.data()), data.size());
            }
            is.close();
        }
    }
    return data;
}

} // namespace vkex::fs

#endif // __VKEX_FILE_SYSTEM_H__
