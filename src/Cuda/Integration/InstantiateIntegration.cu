//
// Created by wei on 10/10/18.
//

#include <Cuda/Container/HashTableCudaDevice.cuh>
#include <Cuda/Container/HashTableCudaKernel.cuh>

#include "UniformTSDFVolumeCudaDevice.cuh"
#include "UniformTSDFVolumeCudaKernel.cuh"
#include "UniformMeshVolumeCudaDevice.cuh"
#include "UniformMeshVolumeCudaKernel.cuh"
#include "ScalableTSDFVolumeCudaDevice.cuh"
#include "ScalableTSDFVolumeCudaKernel.cuh"
#include "ScalableMeshVolumeCudaDevice.cuh"
#include "ScalableMeshVolumeCudaKernel.cuh"

namespace open3d {

namespace cuda {
template
class UniformTSDFVolumeCudaServer<8>;
template
class UniformTSDFVolumeCudaServer<16>;
template
class UniformTSDFVolumeCudaServer<256>;
template
class UniformTSDFVolumeCudaServer<512>;

template
class UniformTSDFVolumeCudaKernelCaller<8>;
template
class UniformTSDFVolumeCudaKernelCaller<16>;
template
class UniformTSDFVolumeCudaKernelCaller<256>;
template
class UniformTSDFVolumeCudaKernelCaller<512>;

template
class UniformMeshVolumeCuda<8>;
template
class UniformMeshVolumeCuda<16>;
template
class UniformMeshVolumeCuda<256>;
template
class UniformMeshVolumeCuda<512>;

template
class UniformMeshVolumeCudaKernelCaller<8>;
template
class UniformMeshVolumeCudaKernelCaller<16>;
template
class UniformMeshVolumeCudaKernelCaller<256>;
template
class UniformMeshVolumeCudaKernelCaller<512>;


/** Scalable part **/
/** Oh we can't afford larger chunks **/
template
class HashTableCudaKernelCaller
    <Vector3i, UniformTSDFVolumeCudaServer<8>, SpatialHasher>;
template
class MemoryHeapCudaKernelCaller<UniformTSDFVolumeCudaServer<8>>;

template
class ScalableTSDFVolumeCudaServer<8>;
template
class ScalableTSDFVolumeCudaKernelCaller<8>;

template
class ScalableMeshVolumeCudaServer<8>;
template
class ScalableMeshVolumeCudaKernelCaller<8>;

} // cuda
} // open3d
