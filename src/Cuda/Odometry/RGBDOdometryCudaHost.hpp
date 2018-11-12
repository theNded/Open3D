//
// Created by wei on 11/9/18.
//

#pragma once

#include "RGBDOdometryCuda.h"
#include <sophus/se3.hpp>

namespace open3d {
/**
 * Client end
 * TODO: Think about how do we use server_ ... we don't want copy
 * constructors for such a large system...
 */
template<size_t N>
RGBDOdometryCuda<N>::RGBDOdometryCuda() {
    server_ = std::make_shared<RGBDOdometryCudaServer<N>>();
}

template<size_t N>
RGBDOdometryCuda<N>::~RGBDOdometryCuda() {
    Release();
    server_ = nullptr;
}

template<size_t N>
void RGBDOdometryCuda<N>::SetParameters(float sigma,
                                        float depth_near_threshold,
                                        float depth_far_threshold,
                                        float depth_diff_threshold) {
    server_->sigma_ = sigma;
    server_->sqrt_coeff_D_ = sqrtf(sigma);
    server_->sqrt_coeff_I_ = sqrtf(1 - sigma);
    server_->depth_diff_threshold_ = depth_near_threshold;
    server_->depth_far_threshold_ = depth_far_threshold;
    server_->depth_diff_threshold_ = depth_diff_threshold;
}

template<size_t N>
void RGBDOdometryCuda<N>::SetIntrinsics(PinholeCameraIntrinsic intrinsics) {
    intrinsics_ = intrinsics;
    server_->intrinsics_[0] = PinholeCameraIntrinsicCuda(intrinsics);
    for (size_t i = 1; i < N; ++i) {
        server_->intrinsics_[i] = server_->intrinsics_[i - 1].Downsample();
    }
}

template<size_t N>
void RGBDOdometryCuda<N>::Create(int width, int height) {
    if (server_ != nullptr) {
        PrintError("[RGBDOdometryCuda] Already created, abort!\n");
        return;
    }

    source_on_target_.Create(width, height);

    source_.Create(width, height);
    target_.Create(width, height);
    target_dx_.Create(width, height);
    target_dy_.Create(width, height);

    results_.Create(29);

    UpdateServer();
}

template<size_t N>
void RGBDOdometryCuda<N>::Release() {
    source_.Release();
    target_.Release();
    target_dx_.Release();
    target_dy_.Release();

    source_on_target_.Release();

    results_.Release();
}

template<size_t N>
void RGBDOdometryCuda<N>::UpdateServer() {
    if (server_ != nullptr) {
        server_->source_on_target() = *source_on_target_.server();

        server_->source() = *source_.server();
        server_->target() = *target_.server();
        server_->target_dx() = *target_dx_.server();
        server_->target_dy() = *target_dy_.server();

        server_->results() = *results_.server();
    }
}

template<size_t N>
void RGBDOdometryCuda<N>::ExtractResults(std::vector<float> &results,
                                         EigenMatrix6d &JtJ,
                                         EigenVector6d &Jtr,
                                         float &error, float &inliers) {
    int cnt = 0;
    for (int i = 0; i < 6; ++i) {
        for (int j = i; j < 6; ++j) {
            JtJ(i, j) = JtJ(j, i) = results[cnt];
            ++cnt;
        }
    }
    for (int i = 0; i < 6; ++i) {
        Jtr(i) = results[cnt];
        ++cnt;
    }
    error = results[cnt];
    ++cnt;
    inliers = results[cnt];
}

template<size_t N>
void RGBDOdometryCuda<N>::PrepareData(
    RGBDImageCuda &source, RGBDImageCuda &target) {

    int source_width = source.depthf().width_;
    int source_height = source.depthf().height_;
    int target_width = target.depthf().width_;
    int target_height = target.depthf().height_;
    assert(source_width > 0 && source_height > 0);
    assert(target_width > 0 && target_height > 0);

    source_.Build(source);
    target_.Build(target);

    target_dx_.Create(source_width, source_height);
    target_dy_.Create(target_width, target_height);
    source_on_target_.Create(source_width, source_height);

    for (size_t i = 0; i < N; ++i) {
        target_[i].depthf().Sobel(target_dx_[i].depthf(),
                                  target_dy_[i].depthf(),
                                  false);
        target_[i].intensity().Sobel(target_dx_[i].intensity(),
                                     target_dy_[i].intensity(),
                                     false);
    }

    target_dx_.UpdateServer();
    target_dy_.UpdateServer();
    source_on_target_.UpdateServer();

    results_.Create(29);
    UpdateServer();
}

template<size_t N>
void RGBDOdometryCuda<N>::Apply() {

    const int kIterations[] = {10, 10, 10};
    for (int level = (int) (N - 1); level >= 0; --level) {
        for (int iter = 0; iter < kIterations[level]; ++iter) {
            results_.Memset(0);

#ifdef VISUALIZE_ODOMETRY_INLIERS
            source_on_target_[level].CopyFrom(target_[level].intensity());
#endif
            server_->transform_source_to_target_.FromEigen(
                transform_source_to_target_);

            RGBDOdometryCudaKernelCaller<N>::ApplyRGBDOdometryKernelCaller(
                *server_, level,
                source_[level].depthf().width_,
                source_[level].depthf().height_);

#ifdef VISUALIZE_ODOMETRY_INLIERS
            cv::Mat im = source_on_target_[level].DownloadMat();
            cv::imshow("source_on_target", im);
            cv::waitKey(-1);
#endif

            std::vector<float> results = results_.DownloadAll();

            EigenMatrix6d JtJ;
            EigenVector6d Jtr;
            float error, inliers;
            ExtractResults(results, JtJ, Jtr, error, inliers);

            PrintDebug("> Level %d, iter %d: error = %f, avg_error = %f, "
                       "inliers = %.0f\n",
                       level, iter, error, error / inliers, inliers);

            EigenVector6d dxi = JtJ.ldlt().solve(-Jtr);
            transform_source_to_target_ =
                Sophus::SE3d::exp(dxi).matrix() * transform_source_to_target_;

        }
    }
}
}
