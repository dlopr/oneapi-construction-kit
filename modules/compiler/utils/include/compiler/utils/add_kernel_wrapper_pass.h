// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Add kernel wrapper pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_ADD_KERNEL_WRAPPER_PASS_H_INCLUDED
#define COMPILER_UTILS_ADD_KERNEL_WRAPPER_PASS_H_INCLUDED

#include <compiler/utils/builtin_info.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

struct AddKernelWrapperPassOptions {
  bool IsPackedStruct = true;
  bool PassLocalBuffersBySize = true;
};

/// @brief A pass that will wrap the kernel with a packed args
/// structure.
///
/// Runs over all kernels with "kernel" metadata.
///
/// The wrappers take the base of their names from the wrapped functions with
/// an additional suffix. The wrapped function's "original function" name is
/// taken if present, else the wrapped functon's name is taken directly.
///
/// Note that if it is not packed it will align each parameter to the next power
/// of 2 up to 128 of the size of the arg when it places it in the structure.
/// This will normally be done using the DataLayout class, but padding will be
/// added as necessary.
class AddKernelWrapperPass final
    : public llvm::PassInfoMixin<AddKernelWrapperPass> {
 public:
  AddKernelWrapperPass(AddKernelWrapperPassOptions Opts)
      : IsPacked(Opts.IsPackedStruct),
        PassLocalBuffersBySize(Opts.PassLocalBuffersBySize) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

  struct KernelArgMapping {
    /// @brief The original index of the argument in the wrapped function.
    unsigned OldArgIdx;
    /// @brief The new index of the argument in the wrapper function. Set to -1
    /// if not present.
    int NewArgIdx = -1;
    /// @brief The scheduling parameter index, indexing into the target's list
    /// of scheduling parameters. Set to -1 if not a scheduling parameter.
    int SchedParamIdx = -1;
    /// @brief The packed-arg struct field index of the argument in the wrapper
    /// function. Set to -1 if not present (i.e., not a packed argument).
    int PackedStructFieldIdx = -1;
  };

 private:
  void createNewFunctionArgTypes(
      llvm::Module &M, const llvm::Function &F,
      const llvm::SmallVectorImpl<compiler::utils::BuiltinInfo::SchedParamInfo>
          &SchedParamInfo,
      llvm::StructType *StructTy,
      llvm::SmallVectorImpl<KernelArgMapping> &ArgMappings,
      llvm::SmallVectorImpl<llvm::Type *> &ArgTypes);

  bool IsPacked;
  bool PassLocalBuffersBySize;
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ADD_KERNEL_WRAPPER_PASS_H_INCLUDED
