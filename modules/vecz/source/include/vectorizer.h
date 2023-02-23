// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file vectorizer.h
///
/// @brief Entry point for the kernel vectorizer.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_VECTORIZER_H_INCLUDED
#define VECZ_VECTORIZER_H_INCLUDED

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class Function;
}  // namespace llvm

namespace vecz {

/// @brief The maximum number of vectorization dimension that Vecz can handle.
///
/// The current limitation is due to the assumption that work groups are
/// being represented as 1- 2- or 3-dimensional arrays or work items.
const unsigned MAX_SIMD_DIM = 3;

class VectorizationContext;
class VectorizationUnit;
struct VeczPassOptions;

/// @brief Try to create a vectorization unit for the given kernel function,
///        with the given vectorization factor and vectorization options.
///
/// @param[in] Ctx VectorizationContext used to perform the vectorization.
/// @param[in] Kernel kernel function to vectorize.
/// @param[in] Opts Vecz Pass Options struct for this vectorization.
/// @param[in] FAM Function Analysis Manager for running analyses
/// @param[in] Check check for vectorizability before creating the VU
///
/// @return Pointer to a vectorization unit on success, or nullptr on failure.
VectorizationUnit *createVectorizationUnit(VectorizationContext &Ctx,
                                           llvm::Function *Kernel,
                                           const VeczPassOptions &Opts,
                                           llvm::FunctionAnalysisManager &FAM,
                                           bool Check);

/// @brief Create metadata for the vectorization unit relating the vectorized
///        function to the scalar function.
///
/// @param[in] VU the vectorization Unit of to create metadata for
/// @returns true iff vectorization succeeded.
bool createVectorizedFunctionMetadata(VectorizationUnit &VU);

/// @brief Register failure, success, and update statistics for the given
/// VectorizationUnit.
///
/// @param[in] VU the vectorization Unit of to create metadata for
/// @returns true iff vectorization succeeded.
void trackVeczSuccessFailure(VectorizationUnit &VU);
}  // namespace vecz

#endif  // VECZ_VECTORIZER_H_INCLUDED
