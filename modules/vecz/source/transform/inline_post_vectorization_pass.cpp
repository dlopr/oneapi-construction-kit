// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "transform/inline_post_vectorization_pass.h"

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/mangling.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>

#include "analysis/vectorization_unit_analysis.h"
#include "debugging.h"
#include "vecz/vecz_choices.h"

using namespace llvm;
using namespace vecz;

namespace {
/// @brief Process a call site, inlining it or marking it as needing inlining
/// if required.
///
/// @param[in] CI Call site to inspect.
/// @param[out] NeedLLVMInline Whether the call site needs LLVM inlining.
/// @param[in] BI Builtin database.
///
/// @return New return value for the call instruction.
Value *processCallSite(CallInst *CI, bool &NeedLLVMInline,
                       compiler::utils::BuiltinInfo &BI) {
  NeedLLVMInline = false;

  Function *Callee = CI->getCalledFunction();
  if (!Callee) {
    return CI;
  }

  // Mark called function as needing inlining by LLVM, unless it has the
  // NoInline attribute
  if (!Callee->isDeclaration() &&
      !Callee->hasFnAttribute(Attribute::NoInline)) {
    CI->addFnAttr(Attribute::AlwaysInline);
    NeedLLVMInline = true;
    return CI;
  }

  // Emit builtins inline when they have no vector/scalar equivalent.
  IRBuilder<> B(CI);
  auto const Builtin = BI.analyzeBuiltin(*Callee);
  if (Builtin.properties &
      compiler::utils::eBuiltinPropertyInlinePostVectorization) {
    SmallVector<Value *, 4> Args(CI->args());
    if (Value *Impl = BI.emitBuiltinInline(Callee, B, Args)) {
      VECZ_ERROR_IF(
          Impl->getType() != CI->getType(),
          "The inlined function type must match that of the original function");
      return Impl;
    }
  }

  // Vectorized uses of the subgroup local id will have been replaced with step
  // vectors starting from zero. Uniform uses should be replaced with zero in
  // order to maintain equivalence between the scalar/vector forms. Do this
  // here due to a tight coupling between the vectorized version and these
  // remaining scalar versions.
  if (Builtin.isValid() && Builtin.ID == BI.getSubgroupLocalIdBuiltin()) {
    return ConstantInt::getNullValue(CI->getType());
  }

  return CI;
}

}  // namespace

PreservedAnalyses InlinePostVectorizationPass::run(
    Function &F, FunctionAnalysisManager &AM) {
  bool modified = false;
  bool needToRunInliner = false;
  auto &BI =
      AM.getResult<VectorizationContextAnalysis>(F).getContext().builtins();

  SmallVector<Instruction *, 4> ToDelete;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      // Look for calls to builtins with no vector/scalar equivalent.
      CallInst *CI = dyn_cast<CallInst>(&I);
      if (!CI) {
        continue;
      }

      bool NeedLLVMInline = false;
      Value *NewCI = processCallSite(CI, NeedLLVMInline, BI);
      needToRunInliner |= NeedLLVMInline;
      if ((NewCI == CI) || !NewCI) {
        continue;
      }

      if (!CI->getType()->isVoidTy()) {
        CI->replaceAllUsesWith(NewCI);
      }
      ToDelete.push_back(CI);
      modified = true;
    }
  }

  // Clean up.
  while (!ToDelete.empty()) {
    Instruction *I = ToDelete.pop_back_val();
    I->eraseFromParent();
  }

  // Run the LLVM inliner if some calls were marked as needing inlining.
  if (needToRunInliner) {
    llvm::legacy::PassManager PM;
    PM.add(llvm::createAlwaysInlinerLegacyPass());
    PM.run(*F.getParent());
    modified = true;
  }

  // Recursively run the pass to inline any newly introduced functions.
  if (modified) {
    run(F, AM);
  }

  return PreservedAnalyses::none();
}
