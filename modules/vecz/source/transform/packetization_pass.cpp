// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "transform/packetization_pass.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>

#include "analysis/control_flow_analysis.h"
#include "analysis/divergence_analysis.h"
#include "analysis/simd_width_analysis.h"
#include "analysis/stride_analysis.h"
#include "analysis/uniform_value_analysis.h"
#include "analysis/vectorization_unit_analysis.h"
#include "debugging.h"
#include "transform/packetizer.h"
#include "vectorization_unit.h"
#include "vecz/vecz_target_info.h"

#define DEBUG_TYPE "vecz-packetization"

using namespace vecz;
using namespace llvm;

STATISTIC(VeczPacketizeFail,
          "Number of kernels that failed to packetize [ID#P80]");
STATISTIC(VeczSimdAnalysisFail,
          "Number of kernels that SIMD Width Analysis "
          "suggested not to packetize [ID#P81]");

char PacketizationPass::PassID = 0;

PreservedAnalyses PacketizationPass::run(Function &F,
                                         llvm::FunctionAnalysisManager &AM) {
  VectorizationUnit &VU = AM.getResult<VectorizationUnitAnalysis>(F).getVU();

  if (!VU.width().isScalable()) {
    unsigned SimdWidth = VU.width().getFixedValue();
    if (VU.autoWidth() && VU.context().targetInfo().getTargetMachine()) {
      LLVM_DEBUG(dbgs() << "vecz: Original SIMD width: " << SimdWidth << "\n");
      unsigned NewSimdWidth = AM.getResult<SimdWidthAnalysis>(F).value;
      LLVM_DEBUG(dbgs() << "vecz: Re-determined SIMD width: " << NewSimdWidth
                        << "\n");

      if (NewSimdWidth <= 1u) {
        ++VeczSimdAnalysisFail;
        return VU.setFailed("SIMD Width Analysis suggested not to packetize");
      }

      if (NewSimdWidth < SimdWidth) {
        VU.setWidth(ElementCount::getFixed(NewSimdWidth));
      }
    }
  }

  if (!Packetizer::packetize(F, AM, VU.width(), VU.dimension())) {
    ++VeczPacketizeFail;
    return VU.setFailed("packetization failed");
  }

  PreservedAnalyses Preserved;
  Preserved.preserve<DominatorTreeAnalysis>();
  Preserved.preserve<LoopAnalysis>();
  Preserved.preserve<CFGAnalysis>();
  Preserved.preserve<DivergenceAnalysis>();
  return Preserved;
}
