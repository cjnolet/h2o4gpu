/*!
 * Copyright 2018 H2O.ai, Inc.
 * License   Apache License Version 2.0 (see LICENSE for details)
 */

#ifndef KM_BLAS_CUH_
#define KM_BLAS_CUH_

#include <cublas_v2.h>
#include "KmConfig.h"

// C++ Wrappers for cublas

namespace H2O4GPU {
namespace KMeans {

namespace Blas {
// LEVEL 1
inline void axpy(cublasHandle_t handle, int n,
                 const float *alpha,
                 const float *x, int incx,
                 float *y, int incy) {
  CUBLAS_CHECK(cublasSaxpy(handle, n,
                           alpha,
                           x, incx,
                           y, incy));}

inline void axpy(cublasHandle_t handle, int n,
                 const double *alpha,
                 const double *x, int incx,
                 double *y, int incy) {
  CUBLAS_CHECK(cublasDaxpy(handle, n,
                           alpha,
                           x, incx,
                           y, incy));}

// LEVEL 3
inline void gemm(cublasHandle_t handle,
                 cublasOperation_t transa,
                 cublasOperation_t transb,
                 int m,
                 int n,
                 int k,
                 const float *alpha, /* host or device pointer */
                 const float *A,
                 int lda,
                 const float *B,
                 int ldb,
                 const float *beta, /* host or device pointer */
                 float *C,
                 int ldc) {
  CUBLAS_CHECK(cublasSgemm(handle,
                           transa, transb,
                           m, n, k,
                           alpha, /* host or device pointer */
                           A, lda,
                           B, ldb,
                           beta, /* host or device pointer */
                           C, ldc));}

inline void gemm(cublasHandle_t handle,
                 cublasOperation_t transa,
                 cublasOperation_t transb,
                 int m,
                 int n,
                 int k,
                 const double *alpha, /* host or device pointer */
                 const double *A,
                 int lda,
                 const double *B,
                 int ldb,
                 const double *beta, /* host or device pointer */
                 double *C,
                 int ldc) {
  CUBLAS_CHECK(cublasDgemm(handle,
                           transa,
                           transb,
                           m,
                           n,
                           k,
                           alpha, /* host or device pointer */
                           A,
                           lda,
                           B,
                           ldb,
                           beta, /* host or device pointer */
                           C,
                           ldc));}

inline void gemm_batched(cublasHandle_t handle,
                         cublasOperation_t transa, 
                         cublasOperation_t transb,
                         int m, int n, int k,
                         const double *alpha,
                         const double *Aarray[], int lda,
                         const double *Barray[], int ldb,
                         const double *beta,
                         double          *Carray[], int ldc, 
                         int batchCount) {
  CUBLAS_CHECK(cublasDgemmBatched(handle,
                                  transa, 
                                  transb,
                                  m, n, k,
                                  alpha,
                                  Aarray, lda,
                                  Barray, ldb,
                                  beta,
                                  Carray, ldc, 
                                  batchCount));
}

inline void gemm_batched(cublasHandle_t handle,
                         cublasOperation_t transa, 
                         cublasOperation_t transb,
                         int m, int n, int k,
                         const float *alpha,
                         const float *Aarray[], int lda,
                         const float *Barray[], int ldb,
                         const float *beta,
                         float *Carray[], int ldc, 
                         int batchCount) {
  CUBLAS_CHECK(cublasSgemmBatched(handle,
                                  transa, 
                                  transb,
                                  m, n, k,
                                  alpha,
                                  Aarray, lda,
                                  Barray, ldb,
                                  beta,
                                  Carray, ldc, 
                                  batchCount));
}

inline void gemm_strided_batched(
    cublasHandle_t handle, 
    cublasOperation_t transA, cublasOperation_t transB,
    int M, int N, int K, 
    const double* alpha,
    const double* A, int ldA, int strideA, 
    const double* B, int ldB, int strideB, 
    const double* beta,
    double* C, int ldC, int strideC,
    int batchCount) {
  CUBLAS_CHECK(cublasDgemmStridedBatched(handle,
                                         transA, 
                                         transB,
                                         M, N, K,
                                         alpha,
                                         A, ldA,
                                         strideA,
                                         B, ldB,
                                         strideB,
                                         beta,
                                         C, ldC, 
                                         strideC, 
                                         batchCount));
}

inline void gemm_strided_batched(
    cublasHandle_t handle, 
    cublasOperation_t transA, cublasOperation_t transB,
    int M, int N, int K, 
    const float* alpha,
    const float* A, int ldA, int strideA, 
    const float* B, int ldB, int strideB, 
    const float* beta,
    float* C, int ldC, int strideC,
    int batchCount) {
  CUBLAS_CHECK(cublasSgemmStridedBatched(handle,
                                         transA, 
                                         transB,
                                         M, N, K,
                                         alpha,
                                         A, ldA,
                                         strideA,
                                         B, ldB,
                                         strideB,
                                         beta,
                                         C, ldC, 
                                         strideC, 
                                         batchCount));
}

}  // Blas
}  // KMeans
}  // H2O4GPU

#endif  // KM_BLAS_CUH_