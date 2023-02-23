// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file vecz_pass_builder.h
///
/// @brief class to initialize a Module Pass Manager to perform vectorization.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_VECZ_PASS_BUILDER_H_INCLUDED
#define VECZ_VECZ_PASS_BUILDER_H_INCLUDED

#include <compiler/utils/pass_machinery.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class Module;
class TargetTransformInfo;
class TargetMachine;
}  // namespace llvm

namespace vecz {
class VectorizationContext;

/// @brief A class that manages the lifetime and initialization of all
/// components required to set up an LLVM pass manager to run Vecz passes.
class VeczPassMachinery final : public compiler::utils::PassMachinery {
 public:
  /// @brief Construct the pass machinery.
  /// The base class method `initialize(TargetInfo)` must also be called.
  ///
  /// @param[in] TM TargetMachine to be used for passes. May be nullptr
  /// @param[in] ctx the vectorization context object for the module.
  /// @param[in] verifyEach true if each pass should be verified
  /// @param[in] debugLogLevel debug logging verbosity.
  VeczPassMachinery(llvm::TargetMachine *TM, VectorizationContext &ctx,
                    bool verifyEach,
                    compiler::utils::DebugLogging debugLogLevel =
                        compiler::utils::DebugLogging::None);

  virtual void registerPasses() override;

 private:
  virtual void addClassToPassNames() override;
  virtual void registerPassCallbacks() override;

  VectorizationContext &Ctx;
};

/// @brief Add the full Vecz pass pipeline to the given pass manager.
///
/// @param[in] PM The Module Pass Manager to build.
/// @return true on success.
bool buildPassPipeline(llvm::ModulePassManager &PM);
}  // namespace vecz

#endif  // VECZ_VECZ_PASS_BUILDER_H_INCLUDED
