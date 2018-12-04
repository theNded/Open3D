//
// Created by wei on 10/1/18.
//

#include "ICRGBDOdometryCudaDevice.cuh"

namespace open3d {
namespace cuda {

template<size_t N>
__global__
void DoSingleIterationKernel(
    ICRGBDOdometryCudaServer<N> odometry, size_t level) {
    /** Add more memory blocks if we have **/
    /** TODO: check this version vs 1 __shared__ array version **/
    __shared__ float local_sum0[THREAD_2D_UNIT * THREAD_2D_UNIT];
    __shared__ float local_sum1[THREAD_2D_UNIT * THREAD_2D_UNIT];
    __shared__ float local_sum2[THREAD_2D_UNIT * THREAD_2D_UNIT];

    const int x = threadIdx.x + blockIdx.x * blockDim.x;
    const int y = threadIdx.y + blockIdx.y * blockDim.y;
    const int tid = threadIdx.x + threadIdx.y * blockDim.x;

    /** Proper initialization **/
    local_sum0[tid] = 0;
    local_sum1[tid] = 0;
    local_sum2[tid] = 0;

    if (x >= odometry.target_[level].depth().width_
        || y >= odometry.target_[level].depth().height_)
        return;

    /** Compute Jacobian and residual -> 9ms **/
    float residual_I, residual_D;
    int x_source, y_source;
    bool mask = odometry.ComputePixelwiseCorrespondenceAndResidual(
        x, y, level,
        x_source, y_source, residual_I, residual_D);

    HessianCuda<6> JtJ;
    Vector6f Jtr;
    if (mask) {
        odometry.correspondences_.push_back(Vector4i(x_source, y_source, x, y));

        /** Access pre-computed pixel-wise jacobians **/
        Vector6f &jacobian_I = odometry.source_intensity_jacobian_[level].at(
            x_source, y_source);
        Vector6f &jacobian_D = odometry.source_depth_jacobian_[level].at(
            x_source, y_source);

        odometry.ComputePixelwiseJtJAndJtr(
            jacobian_I, jacobian_D, residual_I, residual_D,
            JtJ, Jtr);
    }

    /** Reduce Sum JtJ -> 2ms **/
#pragma unroll 1
    for (size_t i = 0; i < 21; i += 3) {
        local_sum0[tid] = mask ? JtJ(i + 0) : 0;
        local_sum1[tid] = mask ? JtJ(i + 1) : 0;
        local_sum2[tid] = mask ? JtJ(i + 2) : 0;
        __syncthreads();

        if (tid < 128) {
            local_sum0[tid] += local_sum0[tid + 128];
            local_sum1[tid] += local_sum1[tid + 128];
            local_sum2[tid] += local_sum2[tid + 128];
        }
        __syncthreads();

        if (tid < 64) {
            local_sum0[tid] += local_sum0[tid + 64];
            local_sum1[tid] += local_sum1[tid + 64];
            local_sum2[tid] += local_sum2[tid + 64];
        }
        __syncthreads();

        if (tid < 32) {
            WarpReduceSum<float>(local_sum0, tid);
            WarpReduceSum<float>(local_sum1, tid);
            WarpReduceSum<float>(local_sum2, tid);
        }

        if (tid == 0) {
            atomicAdd(&odometry.results_.at(i + 0), local_sum0[0]);
            atomicAdd(&odometry.results_.at(i + 1), local_sum1[0]);
            atomicAdd(&odometry.results_.at(i + 2), local_sum2[0]);
        }
        __syncthreads();
    }

    /** Reduce Sum Jtr **/
#define OFFSET1 21
#pragma unroll 1
    for (size_t i = 0; i < 6; i += 3) {
        local_sum0[tid] = mask ? Jtr(i + 0) : 0;
        local_sum1[tid] = mask ? Jtr(i + 1) : 0;
        local_sum2[tid] = mask ? Jtr(i + 2) : 0;
        __syncthreads();

        if (tid < 128) {
            local_sum0[tid] += local_sum0[tid + 128];
            local_sum1[tid] += local_sum1[tid + 128];
            local_sum2[tid] += local_sum2[tid + 128];
        }
        __syncthreads();

        if (tid < 64) {
            local_sum0[tid] += local_sum0[tid + 64];
            local_sum1[tid] += local_sum1[tid + 64];
            local_sum2[tid] += local_sum2[tid + 64];
        }
        __syncthreads();

        if (tid < 32) {
            WarpReduceSum<float>(local_sum0, tid);
            WarpReduceSum<float>(local_sum1, tid);
            WarpReduceSum<float>(local_sum2, tid);
        }

        if (tid == 0) {
            atomicAdd(&odometry.results_.at(i + 0 + OFFSET1), local_sum0[0]);
            atomicAdd(&odometry.results_.at(i + 1 + OFFSET1), local_sum1[0]);
            atomicAdd(&odometry.results_.at(i + 2 + OFFSET1), local_sum2[0]);
        }
        __syncthreads();
    }

    /** Reduce Sum loss and inlier **/
#define OFFSET2 27
    {
        local_sum0[tid] = mask ?
                          residual_I * residual_I + residual_D * residual_D : 0;
        local_sum1[tid] = mask ? 1 : 0;
        __syncthreads();

        if (tid < 128) {
            local_sum0[tid] += local_sum0[tid + 128];
            local_sum1[tid] += local_sum1[tid + 128];
        }
        __syncthreads();

        if (tid < 64) {
            local_sum0[tid] += local_sum0[tid + 64];
            local_sum1[tid] += local_sum1[tid + 64];
        }
        __syncthreads();

        if (tid < 32) {
            WarpReduceSum<float>(local_sum0, tid);
            WarpReduceSum<float>(local_sum1, tid);
        }

        if (tid == 0) {
            atomicAdd(&odometry.results_.at(0 + OFFSET2), local_sum0[0]);
            atomicAdd(&odometry.results_.at(1 + OFFSET2), local_sum1[0]);
        }
        __syncthreads();
    }
}

template<size_t N>
void ICRGBDOdometryCudaKernelCaller<N>::DoSinlgeIterationKernelCaller(
    ICRGBDOdometryCudaServer<N> &server, size_t level,
    int width, int height) {

    const dim3 blocks(DIV_CEILING(width, THREAD_2D_UNIT),
                      DIV_CEILING(height, THREAD_2D_UNIT));
    const dim3 threads(THREAD_2D_UNIT, THREAD_2D_UNIT);
    DoSingleIterationKernel << < blocks, threads >> > (server, level);
    CheckCuda(cudaDeviceSynchronize());
    CheckCuda(cudaGetLastError());
}

template<size_t N>
__global__
void PrecomputeJacobiansKernel(
    ICRGBDOdometryCudaServer<N> odometry, size_t level) {
    const int x = threadIdx.x + blockIdx.x * blockDim.x;
    const int y = threadIdx.y + blockIdx.y * blockDim.y;

    if (x >= odometry.source_[level].depth().width_
        || y >= odometry.source_[level].depth().height_)
        return;

    odometry.ComputePixelwiseJacobian(x, y, level);
}

template<size_t N>
void ICRGBDOdometryCudaKernelCaller<N>::PrecomputeJacobiansKernelCaller(
    ICRGBDOdometryCudaServer<N> &server, size_t level,
    int width, int height) {

    const dim3 blocks(DIV_CEILING(width, THREAD_2D_UNIT),
                      DIV_CEILING(height, THREAD_2D_UNIT));
    const dim3 threads(THREAD_2D_UNIT, THREAD_2D_UNIT);
    PrecomputeJacobiansKernel << < blocks, threads >> > (server, level);
    CheckCuda(cudaDeviceSynchronize());
    CheckCuda(cudaGetLastError());
}
} // cuda
} // open3d