// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "Hashmap.h"
#include <unordered_map>

namespace open3d {

std::shared_ptr<CUDAHashmap> CreateCUDAHashmap(uint32_t max_keys,
                                               uint32_t dsize_key,
                                               uint32_t dsize_value,
                                               open3d::Device device) {
    return std::make_shared<CUDAHashmap>(max_keys, dsize_key, dsize_value,
                                         device);
}

std::shared_ptr<Hashmap> CreateHashmap(uint32_t max_keys,
                                       uint32_t dsize_key,
                                       uint32_t dsize_value,
                                       open3d::Device device) {
    static std::unordered_map<
            open3d::Device::DeviceType,
            std::function<std::shared_ptr<Hashmap>(uint32_t, uint32_t, uint32_t,
                                                   open3d::Device)>,
            open3d::utility::hash_enum_class::hash>
            map_device_type_to_memory_manager = {
                    {open3d::Device::DeviceType::CUDA, CreateCUDAHashmap}};

    if (map_device_type_to_memory_manager.find(device.GetType()) ==
        map_device_type_to_memory_manager.end()) {
        open3d::utility::LogError(
                "MemoryManager::GetDeviceMemoryManager: Unimplemented device");
    }

    auto fn = map_device_type_to_memory_manager.at(device.GetType());
    return fn(max_keys, dsize_key, dsize_value, device);
}
}  // namespace open3d
