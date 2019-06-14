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

#pragma once

#include <stdexcept>
#include <string>

// TODO: - stateful memory manager
//       - consider memory pool approach
class MemoryManager {
public:
    static void* Allocate(size_t byte_size, const std::string& device) {
        if (device == "cpu") {
            void* ptr = malloc(byte_size);
            if (byte_size != 0 && !ptr) {
                std::runtime_error("CPU malloc failed");
                throw std::bad_alloc();
            }
            return ptr;
        } else if (device == "gpu") {
            throw std::runtime_error("Unimplemented");
        } else {
            throw std::runtime_error("Unrecognized device");
        }
    }

    // TODO: consider removing the "device" argument, check ptr device first
    static void Free(void* ptr, const std::string& device) {
        if (device == "cpu") {
            if (ptr) {
                free(ptr);
            }
        } else if (device == "gpu") {
            // cudaPointerAttributes attributes;
            // CUDA_RT_SAFE_CALL_NO_THROW(
            //         cudaPointerGetAttributes(&attributes, ptr));
            // if (attributes.devicePointer != nullptr) {
            //     return true;
            // }
            // return false;
            throw std::runtime_error("Unimplemented");
        } else {
            throw std::runtime_error("Unrecognized device");
        }
    }

    static void CopyTo(void* dst_ptr,
                       const void* src_ptr,
                       const std::string& dst_device,
                       const std::string& src_device) {}
};
