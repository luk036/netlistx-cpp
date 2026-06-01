/**
 * @file rand_cover_gpu.cu
 * @brief CUDA kernel and host-side launch for Pitt's randomized vertex cover.
 *
 * Compiled only when HAS_CUDA is defined (by nvcc).
 * Provides:
 *   - pitt_kernel: CUDA kernel (each thread = one Pitt trial)
 *   - run_gpu_trials(): host-side function callable from MSVC-compiled code
 */

#include <cuda_runtime.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

// ============================================================================
// CUDA Kernel -- each thread runs one independent Pitt trial
// ============================================================================

extern "C" __global__ void pitt_kernel(const int* edges, int num_edges,
                                        const float* weights, int num_vertices,
                                        unsigned int* covers, float* costs,
                                        unsigned long long* seeds,
                                        int num_trials) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_trials) return;

    unsigned long long seed = seeds[tid];
    const int num_words = (num_vertices + 31) / 32;
    unsigned int* cover = &covers[tid * num_words];

    for (int i = 0; i < num_edges; ++i) {
        const int u = edges[i * 2];
        const int v = edges[i * 2 + 1];

        const int u_word = u >> 5;
        const int v_word = v >> 5;
        const unsigned int u_bit = 1u << (u & 31);
        const unsigned int v_bit = 1u << (v & 31);

        if ((cover[u_word] & u_bit) == 0 && (cover[v_word] & v_bit) == 0) {
            seed = (seed * 1103515245ULL + 12345ULL) & 0x7FFFFFFFULL;
            const float rand_val =
                static_cast<float>(seed) / 2147483648.0f;

            const float w_u = weights[u];
            const float w_v = weights[v];
            const float threshold = w_v / (w_u + w_v);

            if (rand_val < threshold) {
                cover[u_word] |= u_bit;
            } else {
                cover[v_word] |= v_bit;
            }
        }
    }

    float cost = 0.0f;
    for (int v = 0; v < num_vertices; ++v) {
        if (cover[v >> 5] & (1u << (v & 31))) {
            cost += weights[v];
        }
    }
    costs[tid] = cost;
}

// ============================================================================
// Host-side CUDA launch -- compiled by nvcc, callable from MSVC code
// ============================================================================

#define CUDA_CHECK(call)                                            \
    do {                                                             \
        cudaError_t err_ = call;                                     \
        if (err_ != cudaSuccess) {                                   \
            std::cerr << "CUDA error: " << cudaGetErrorString(err_)  \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1);                                            \
        }                                                            \
    } while (0)

extern "C" void run_gpu_trials_raw(
    const int* edges_flat, int num_edges,
    const float* weights_flat, int num_vertices,
    const unsigned long long* seeds, int num_trials,
    int num_words,
    unsigned int* out_covers, float* out_costs) {

    static constexpr int THREADS_PER_BLOCK = 64;

    int* d_edges = nullptr;
    float* d_weights = nullptr;
    unsigned int* d_covers = nullptr;
    float* d_costs = nullptr;
    unsigned long long* d_seeds = nullptr;

    const auto cover_bytes =
        static_cast<std::size_t>(num_words) * static_cast<std::size_t>(num_trials);

    CUDA_CHECK(cudaMalloc(&d_edges, static_cast<std::size_t>(num_edges) * 2 * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_weights, static_cast<std::size_t>(num_vertices) * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_covers, cover_bytes * sizeof(unsigned int)));
    CUDA_CHECK(cudaMalloc(&d_costs, static_cast<std::size_t>(num_trials) * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_seeds, static_cast<std::size_t>(num_trials) * sizeof(unsigned long long)));

    CUDA_CHECK(cudaMemcpy(d_edges, edges_flat, static_cast<std::size_t>(num_edges) * 2 * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_weights, weights_flat, static_cast<std::size_t>(num_vertices) * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_seeds, seeds, static_cast<std::size_t>(num_trials) * sizeof(unsigned long long), cudaMemcpyHostToDevice));

    std::vector<unsigned int> covers_host(cover_bytes, 0u);
    CUDA_CHECK(cudaMemcpy(d_covers, covers_host.data(), cover_bytes * sizeof(unsigned int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(d_costs, 0, static_cast<std::size_t>(num_trials) * sizeof(float)));

    const int grid_blocks = (num_trials + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    pitt_kernel<<<grid_blocks, THREADS_PER_BLOCK>>>(
        d_edges, num_edges, d_weights, num_vertices, d_covers, d_costs, d_seeds, num_trials);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(out_covers, d_covers, cover_bytes * sizeof(unsigned int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(out_costs, d_costs, static_cast<std::size_t>(num_trials) * sizeof(float), cudaMemcpyDeviceToHost));

    cudaFree(d_edges);
    cudaFree(d_weights);
    cudaFree(d_covers);
    cudaFree(d_costs);
    cudaFree(d_seeds);

    std::cerr << "[rand_cover_gpu] GPU confirmed kernel launched (" << num_trials << " trials)\n";
}
