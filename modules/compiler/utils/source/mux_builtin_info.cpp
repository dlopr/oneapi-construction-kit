// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>

using namespace llvm;

namespace compiler {
namespace utils {

namespace SchedParamIndices {
enum {
  WI = 0,
  WG = 1,
  TOTAL,
};
}

static Function *defineLocalWorkItemBuiltin(BIMuxInfoConcept &BI, BuiltinID ID,
                                            Module &M) {
  // Simple 'local' work-item getters and setters.
  bool IsSetter = false;
  bool HasRankArg = false;
  Optional<WorkItemInfoStructField::Type> WIFieldIdx;
  switch (ID) {
    default:
      return nullptr;
    case eMuxBuiltinSetLocalId:
      IsSetter = true;
      LLVM_FALLTHROUGH;
    case eMuxBuiltinGetLocalId:
      HasRankArg = true;
      WIFieldIdx = WorkItemInfoStructField::local_id;
      break;
    case eMuxBuiltinSetSubGroupId:
      IsSetter = true;
      LLVM_FALLTHROUGH;
    case eMuxBuiltinGetSubGroupId:
      WIFieldIdx = WorkItemInfoStructField::sub_group_id;
      break;
    case eMuxBuiltinSetNumSubGroups:
      IsSetter = true;
      LLVM_FALLTHROUGH;
    case eMuxBuiltinGetNumSubGroups:
      WIFieldIdx = WorkItemInfoStructField::num_sub_groups;
      break;
    case eMuxBuiltinSetMaxSubGroupSize:
      IsSetter = true;
      LLVM_FALLTHROUGH;
    case eMuxBuiltinGetMaxSubGroupSize:
      WIFieldIdx = WorkItemInfoStructField::max_sub_group_size;
      break;
  }

  Function *F = M.getFunction(BuiltinInfo::getMuxBuiltinName(ID));
  assert(F && WIFieldIdx);

  // Gather up the list of scheduling parameters on this builtin
  const auto &SchedParams = BI.getFunctionSchedulingParameters(*F);
  assert(SchedParamIndices::WI < SchedParams.size());

  // Grab the work-item info argument
  const auto &SchedParam = SchedParams[SchedParamIndices::WI];
  auto *const StructTy = dyn_cast<StructType>(SchedParam.ParamPointeeTy);
  assert(SchedParam.ArgVal && StructTy == getWorkItemInfoStructTy(M) &&
         "Inconsistent scheduling parameter data");

  if (IsSetter) {
    populateStructSetterFunction(*F, *SchedParam.ArgVal, StructTy, *WIFieldIdx,
                                 HasRankArg);
  } else {
    populateStructGetterFunction(*F, *SchedParam.ArgVal, StructTy, *WIFieldIdx,
                                 HasRankArg);
  }

  return F;
}

static Function *defineLocalWorkGroupBuiltin(BIMuxInfoConcept &BI, BuiltinID ID,
                                             Module &M) {
  // Simple work-group getters
  bool HasRankArg = true;
  size_t DefaultVal = 0;
  Optional<WorkGroupInfoStructField::Type> WGFieldIdx;
  switch (ID) {
    default:
      return nullptr;
    case eMuxBuiltinGetLocalSize:
      DefaultVal = 1;
      WGFieldIdx = WorkGroupInfoStructField::local_size;
      break;
    case eMuxBuiltinGetGroupId:
      DefaultVal = 0;
      WGFieldIdx = WorkGroupInfoStructField::group_id;
      break;
    case eMuxBuiltinGetNumGroups:
      DefaultVal = 1;
      WGFieldIdx = WorkGroupInfoStructField::num_groups;
      break;
    case eMuxBuiltinGetGlobalOffset:
      DefaultVal = 0;
      WGFieldIdx = WorkGroupInfoStructField::global_offset;
      break;
    case eMuxBuiltinGetWorkDim:
      DefaultVal = 1;
      HasRankArg = false;
      WGFieldIdx = WorkGroupInfoStructField::work_dim;
      break;
  }

  Function *F = M.getFunction(BuiltinInfo::getMuxBuiltinName(ID));
  assert(F && WGFieldIdx);

  // Gather up the list of scheduling parameters on this builtin
  const auto &SchedParams = BI.getFunctionSchedulingParameters(*F);
  assert(SchedParamIndices::WG < SchedParams.size());

  // Grab the work-group info argument
  const auto &SchedParam = SchedParams[SchedParamIndices::WG];
  auto *const StructTy = dyn_cast<StructType>(SchedParam.ParamPointeeTy);
  assert(SchedParam.ArgVal && StructTy == getWorkGroupInfoStructTy(M) &&
         "Inconsistent scheduling parameter data");

  populateStructGetterFunction(*F, *SchedParam.ArgVal, StructTy, *WGFieldIdx,
                               HasRankArg, DefaultVal);
  return F;
}

static Value *createCallHelper(IRBuilder<> &B, Function &F,
                               ArrayRef<Value *> Args) {
  auto *const CI = B.CreateCall(&F, Args);
  CI->setAttributes(F.getAttributes());
  CI->setCallingConv(F.getCallingConv());
  return CI;
}

void BIMuxInfoConcept::setDefaultBuiltinAttributes(Function &F,
                                                   bool AlwaysInline) {
  // Many of our mux builtin functions are marked alwaysinline (unless they're
  // already marked noinline)
  if (AlwaysInline && !F.hasFnAttribute(Attribute::NoInline)) {
    F.addFnAttr(Attribute::AlwaysInline);
  }
  // We never use exceptions
  F.addFnAttr(Attribute::NoUnwind);
  // Recursion is not supported in ComputeMux
  F.addFnAttr(Attribute::NoRecurse);
}

Function *BIMuxInfoConcept::defineGetGlobalId(Module &M) {
  Function *F =
      M.getFunction(BuiltinInfo::getMuxBuiltinName(eMuxBuiltinGetGlobalId));
  assert(F);
  setDefaultBuiltinAttributes(*F);
  F->setLinkage(GlobalValue::InternalLinkage);

  // Create an IR builder with a single basic block in our function
  IRBuilder<> B(BasicBlock::Create(M.getContext(), "entry", F));

  auto *const MuxGetGroupIdFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetGroupId, M);
  auto *const MuxGetGlobalOffsetFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetGlobalOffset, M);
  auto *const MuxGetLocalIdFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetLocalId, M);
  auto *const MuxGetLocalSizeFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetLocalSize, M);
  assert(MuxGetGroupIdFn && MuxGetGlobalOffsetFn && MuxGetLocalIdFn &&
         MuxGetLocalSizeFn);

  // Pass on all arguments through to dependent builtins. We expect that each
  // function has identical prototypes, regardless of whether scheduling
  // parameters have been added
  SmallVector<Value *, 4> Args(make_pointer_range(F->args()));

  auto *const GetGroupIdCall = createCallHelper(B, *MuxGetGroupIdFn, Args);
  auto *const GetGlobalOffsetCall =
      createCallHelper(B, *MuxGetGlobalOffsetFn, Args);
  auto *const GetLocalIdCall = createCallHelper(B, *MuxGetLocalIdFn, Args);
  auto *const GetLocalSizeCall = createCallHelper(B, *MuxGetLocalSizeFn, Args);

  // (get_group_id(i) * get_local_size(i))
  auto *Ret = B.CreateMul(GetGroupIdCall, GetLocalSizeCall);
  // (get_group_id(i) * get_local_size(i)) + get_local_id(i)
  Ret = B.CreateAdd(Ret, GetLocalIdCall);
  // get_global_id(i) = (get_group_id(i) * get_local_size(i)) +
  //                    get_local_id(i) + get_global_offset(i)
  Ret = B.CreateAdd(Ret, GetGlobalOffsetCall);

  // ... and return our result
  B.CreateRet(Ret);
  return F;
}

Function *BIMuxInfoConcept::defineGetGlobalSize(Module &M) {
  Function *F =
      M.getFunction(BuiltinInfo::getMuxBuiltinName(eMuxBuiltinGetGlobalSize));
  assert(F);
  setDefaultBuiltinAttributes(*F);
  F->setLinkage(GlobalValue::InternalLinkage);

  auto *const MuxGetNumGroupsFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetNumGroups, M);
  auto *const MuxGetLocalSizeFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetLocalSize, M);
  assert(MuxGetNumGroupsFn && MuxGetLocalSizeFn);

  // create an IR builder with a single basic block in our function
  IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

  // Pass on all arguments through to dependent builtins. We expect that each
  // function has identical prototypes, regardless of whether scheduling
  // parameters have been added
  SmallVector<Value *, 4> Args(make_pointer_range(F->args()));

  // call get_num_groups
  auto *const GetNumGroupsCall = createCallHelper(B, *MuxGetNumGroupsFn, Args);

  // call get_local_size
  auto *const GetLocalSizeCall = createCallHelper(B, *MuxGetLocalSizeFn, Args);

  // get_global_size(i) = get_num_groups(i) * get_local_size(i)
  auto *const Ret = B.CreateMul(GetNumGroupsCall, GetLocalSizeCall);

  // and return our result
  B.CreateRet(Ret);
  return F;
}

Function *BIMuxInfoConcept::defineGetLocalLinearId(Module &M) {
  Function *F = M.getFunction(
      BuiltinInfo::getMuxBuiltinName(eMuxBuiltinGetLocalLinearId));
  assert(F);
  setDefaultBuiltinAttributes(*F);
  F->setLinkage(GlobalValue::InternalLinkage);

  auto *const MuxGetLocalIdFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetLocalId, M);
  auto *const MuxGetLocalSizeFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetLocalSize, M);
  assert(MuxGetLocalIdFn && MuxGetLocalSizeFn);

  // Create a call to all the required builtins.
  IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

  // Pass on all arguments through to dependent builtins. Ignoring the index
  // parameters we'll add, we expect that each function has identical
  // prototypes, regardless of whether scheduling parameters have been added
  SmallVector<Value *, 4> Args(make_pointer_range(F->args()));

  SmallVector<Value *, 4> Idx0Args = {B.getInt32(0)};
  append_range(Idx0Args, Args);
  SmallVector<Value *, 4> Idx1Args = {B.getInt32(1)};
  append_range(Idx1Args, Args);
  SmallVector<Value *, 4> Idx2Args = {B.getInt32(2)};
  append_range(Idx2Args, Args);

  auto *const GetLocalIDXCall = createCallHelper(B, *MuxGetLocalIdFn, Idx0Args);
  auto *const GetLocalIDYCall = createCallHelper(B, *MuxGetLocalIdFn, Idx1Args);
  auto *const GetLocalIDZCall = createCallHelper(B, *MuxGetLocalIdFn, Idx2Args);

  auto *const GetLocalSizeXCall =
      createCallHelper(B, *MuxGetLocalSizeFn, Idx0Args);
  auto *const GetLocalSizeYCall =
      createCallHelper(B, *MuxGetLocalSizeFn, Idx1Args);

  // get_local_id(2) * get_local_size(1).
  auto *ZTerm = B.CreateMul(GetLocalIDZCall, GetLocalSizeYCall);
  // get_local_id(2) * get_local_size(1) * get_local_size(0).
  ZTerm = B.CreateMul(ZTerm, GetLocalSizeXCall);

  // get_local_id(1) * get_local_size(0).
  auto *const YTerm = B.CreateMul(GetLocalIDYCall, GetLocalSizeXCall);

  // get_local_id(2) * get_local_size(1) * get_local_size(0) +
  // get_local_id(1) * get_local_size(0).
  auto *Ret = B.CreateAdd(ZTerm, YTerm);
  // get_local_id(2) * get_local_size(1) * get_local_size(0) +
  // get_local_id(1) * get_local_size(0) + get_local_id(0).
  Ret = B.CreateAdd(Ret, GetLocalIDXCall);

  B.CreateRet(Ret);
  return F;
}

Function *BIMuxInfoConcept::defineGetGlobalLinearId(Module &M) {
  Function *F = M.getFunction(
      BuiltinInfo::getMuxBuiltinName(eMuxBuiltinGetGlobalLinearId));
  assert(F);
  setDefaultBuiltinAttributes(*F);
  F->setLinkage(GlobalValue::InternalLinkage);

  auto *const MuxGetGlobalIdFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetGlobalId, M);
  auto *const MuxGetGlobalOffsetFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetGlobalOffset, M);
  auto *const MuxGetGlobalSizeFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetGlobalSize, M);
  assert(MuxGetGlobalIdFn && MuxGetGlobalOffsetFn && MuxGetGlobalSizeFn);

  // Create a call to all the required builtins.
  IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

  // Pass on all arguments through to dependent builtins. Ignoring the index
  // parameters we'll add, we expect that each function has identical
  // prototypes, regardless of whether scheduling parameters have been added
  SmallVector<Value *, 4> Args(make_pointer_range(F->args()));

  SmallVector<Value *, 4> Idx0Args = {B.getInt32(0)};
  append_range(Idx0Args, Args);
  SmallVector<Value *, 4> Idx1Args = {B.getInt32(1)};
  append_range(Idx1Args, Args);
  SmallVector<Value *, 4> Idx2Args = {B.getInt32(2)};
  append_range(Idx2Args, Args);

  auto *const GetGlobalIDXCall =
      createCallHelper(B, *MuxGetGlobalIdFn, Idx0Args);
  auto *const GetGlobalIDYCall =
      createCallHelper(B, *MuxGetGlobalIdFn, Idx1Args);
  auto *const GetGlobalIDZCall =
      createCallHelper(B, *MuxGetGlobalIdFn, Idx2Args);

  auto *const GetGlobalOffsetXCall =
      createCallHelper(B, *MuxGetGlobalOffsetFn, Idx0Args);
  auto *const GetGlobalOffsetYCall =
      createCallHelper(B, *MuxGetGlobalOffsetFn, Idx1Args);
  auto *const GetGlobalOffsetZCall =
      createCallHelper(B, *MuxGetGlobalOffsetFn, Idx2Args);

  auto *const GetGlobalSizeXCall =
      createCallHelper(B, *MuxGetGlobalSizeFn, Idx0Args);
  auto *const GetGlobalSizeYCall =
      createCallHelper(B, *MuxGetGlobalSizeFn, Idx1Args);

  // global linear id is calculated as follows:
  // get_global_linear_id() =
  // (get_global_id(2) - get_global_offset(2)) * get_global_size(1) *
  // get_global_size(0) + (get_global_id(1) - get_global_offset(1)) *
  // get_global_size(0) + get_global_id(0) - get_global_offset(0).
  // =
  // ((get_global_id(2) - get_global_offset(2)) * get_global_size(1) +
  // get_global_id(1) - get_global_offset(1)) * get_global_size(0) +
  // get_global_id(0) - get_global_offset(0).

  auto *ZTerm = B.CreateSub(GetGlobalIDZCall, GetGlobalOffsetZCall);
  // (get_global_id(2) - get_global_offset(2)) * get_global_size(1).
  ZTerm = B.CreateMul(ZTerm, GetGlobalSizeYCall);

  // get_global_id(1) - get_global_offset(1).
  auto *const YTerm = B.CreateSub(GetGlobalIDYCall, GetGlobalOffsetYCall);

  // (get_global_id(2) - get_global_offset(2)) * get_global_size(1) +
  // get_global_id(1) - get_global_offset(1)
  auto *YZTermsCombined = B.CreateAdd(ZTerm, YTerm);

  // ((get_global_id(2) - get_global_offset(2)) * get_global_size(1) +
  // get_global_id(1) - get_global_offset(1)) * get_global_size(0).
  YZTermsCombined = B.CreateMul(YZTermsCombined, GetGlobalSizeXCall);

  // get_global_id(0) - get_global_offset(0).
  auto *const XTerm = B.CreateSub(GetGlobalIDXCall, GetGlobalOffsetXCall);

  // ((get_global_id(2) - get_global_offset(2)) * get_global_size(1) +
  // get_global_id(1) - get_global_offset(1)) * get_global_size(0) +
  // get_global_id(0) - get_global_offset(0).
  auto *const Ret = B.CreateAdd(XTerm, YZTermsCombined);

  B.CreateRet(Ret);
  return F;
}

Function *BIMuxInfoConcept::defineGetEnqueuedLocalSize(Module &M) {
  Function *F = M.getFunction(
      BuiltinInfo::getMuxBuiltinName(eMuxBuiltinGetEnqueuedLocalSize));
  assert(F);
  setDefaultBuiltinAttributes(*F);
  F->setLinkage(GlobalValue::InternalLinkage);

  auto *const MuxGetLocalSizeFn =
      getOrDeclareMuxBuiltin(eMuxBuiltinGetLocalSize, M);
  assert(MuxGetLocalSizeFn);

  IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

  // Pass on all arguments through to dependent builtins. We expect that each
  // function has identical prototypes, regardless of whether scheduling
  // parameters have been added
  SmallVector<Value *, 4> Args(make_pointer_range(F->args()));

  // Since we don't support non-uniform subgroups
  // get_enqueued_local_size(x) == get_local_size(x).
  auto *const GetLocalSize = createCallHelper(B, *MuxGetLocalSizeFn, Args);

  B.CreateRet(GetLocalSize);
  return F;
}

Function *BIMuxInfoConcept::defineMemBarrier(Function &F, unsigned,
                                             unsigned SemanticsIdx) {
  // FIXME: We're ignoring some operands here. We're dropping the 'scope' but
  // our set of default set of targets can't make use of anything but a
  // single-threaded fence. We're also ignoring the kind of memory being
  // controlled by the barrier.
  // See CA-2997 and CA-3042 for related discussions.
  auto &M = *F.getParent();
  setDefaultBuiltinAttributes(F);
  F.setLinkage(GlobalValue::InternalLinkage);
  IRBuilder<> B(BasicBlock::Create(M.getContext(), "", &F));

  // Grab the semantics argument.
  Value *Semantics = F.getArg(SemanticsIdx);
  // Mask out only the memory ordering value.
  Semantics = B.CreateAnd(Semantics, B.getInt32(MemSemanticsMask));

  // Don't insert this exit block just yet
  auto *const ExitBB = BasicBlock::Create(M.getContext(), "exit");

  auto *const DefaultBB =
      BasicBlock::Create(M.getContext(), "case.default", &F);
  auto *const Switch = B.CreateSwitch(Semantics, DefaultBB);

  struct {
    StringRef Name;
    unsigned SwitchVal;
    AtomicOrdering Ordering;
  } Data[4] = {
      {"case.acquire", MemSemanticsAcquire, AtomicOrdering::Acquire},
      {"case.release", MemSemanticsRelease, AtomicOrdering::Release},
      {"case.acq_rel", MemSemanticsAcquireRelease,
       AtomicOrdering::AcquireRelease},
      {"case.seq_cst", MemSemanticsSequentiallyConsistent,
       AtomicOrdering::SequentiallyConsistent},
  };

  for (const auto &D : Data) {
    auto *const BB = BasicBlock::Create(M.getContext(), D.Name, &F);

    Switch->addCase(B.getInt32(D.SwitchVal), BB);
    B.SetInsertPoint(BB);
    B.CreateFence(D.Ordering, SyncScope::SingleThread);
    B.CreateBr(ExitBB);
  }

  // The default case assumes a 'relaxed' ordering and emits no fence
  // whatsoever.
  B.SetInsertPoint(DefaultBB);
  B.CreateBr(ExitBB);

  ExitBB->insertInto(&F);
  B.SetInsertPoint(ExitBB);
  B.CreateRetVoid();

  return &F;
}

Function *BIMuxInfoConcept::defineMuxBuiltin(BuiltinID ID, Module &M) {
  assert(BuiltinInfo::isMuxBuiltinID(ID) && "Only handling mux builtins");
  Function *F = M.getFunction(BuiltinInfo::getMuxBuiltinName(ID));
  // FIXME: We'd ideally want to declare it here to reduce pass
  // inter-dependencies.
  assert(F && "Function should have been pre-declared");
  if (!F->isDeclaration()) {
    return F;
  }

  switch (ID) {
    default:
      break;
    case eMuxBuiltinGetGlobalId:
      return defineGetGlobalId(M);
    case eMuxBuiltinGetGlobalSize:
      return defineGetGlobalSize(M);
    case eMuxBuiltinGetLocalLinearId:
      return defineGetLocalLinearId(M);
    case eMuxBuiltinGetGlobalLinearId:
      return defineGetGlobalLinearId(M);
    case eMuxBuiltinGetEnqueuedLocalSize:
      return defineGetEnqueuedLocalSize(M);
    // Just handle the memory synchronization requirements of any barrier
    // builtin. We assume that the control requirements of work-group and
    // sub-group control barriers have been handled by earlier passes.
    case eMuxBuiltinMemBarrier:
      return defineMemBarrier(*F, 0, 1);
    case eMuxBuiltinSubGroupBarrier:
    case eMuxBuiltinWorkGroupBarrier:
      return defineMemBarrier(*F, 1, 2);
  }

  if (auto *const NewF = defineLocalWorkItemBuiltin(*this, ID, M)) {
    return NewF;
  }

  if (auto *const NewF = defineLocalWorkGroupBuiltin(*this, ID, M)) {
    return NewF;
  }

  return nullptr;
}

bool BIMuxInfoConcept::requiresSchedulingParameters(BuiltinID ID) {
  switch (ID) {
    default:
      return false;
    case eMuxBuiltinGetLocalId:
    case eMuxBuiltinSetLocalId:
    case eMuxBuiltinGetSubGroupId:
    case eMuxBuiltinSetSubGroupId:
    case eMuxBuiltinGetNumSubGroups:
    case eMuxBuiltinSetNumSubGroups:
    case eMuxBuiltinGetMaxSubGroupSize:
    case eMuxBuiltinSetMaxSubGroupSize:
    case eMuxBuiltinGetLocalLinearId:
      // Work-item struct only
      return true;
    case eMuxBuiltinGetWorkDim:
    case eMuxBuiltinGetGroupId:
    case eMuxBuiltinGetNumGroups:
    case eMuxBuiltinGetGlobalSize:
    case eMuxBuiltinGetLocalSize:
    case eMuxBuiltinGetGlobalOffset:
    case eMuxBuiltinGetEnqueuedLocalSize:
      // Work-group struct only
      return true;
    case eMuxBuiltinGetGlobalId:
    case eMuxBuiltinGetGlobalLinearId:
      // Work-item and work-group structs
      return true;
  }
}

Function *BIMuxInfoConcept::getOrDeclareMuxBuiltin(BuiltinID ID, Module &M) {
  assert(BuiltinInfo::isMuxBuiltinID(ID) && "Only handling mux builtins");
  auto FnName = BuiltinInfo::getMuxBuiltinName(ID);
  if (auto *const F = M.getFunction(FnName)) {
    return F;
  }
  AttrBuilder AB(M.getContext());
  auto *const SizeTy = getSizeType(M);
  auto *const Int32Ty = Type::getInt32Ty(M.getContext());
  auto *const VoidTy = Type::getVoidTy(M.getContext());

  Type *RetTy = nullptr;
  SmallVector<Type *, 4> ParamTys;
  SmallVector<std::string, 4> ParamNames;

  switch (ID) {
    default:
      return nullptr;
    // Ranked Getters
    case eMuxBuiltinGetLocalId:
    case eMuxBuiltinGetGlobalId:
    case eMuxBuiltinGetLocalSize:
    case eMuxBuiltinGetGlobalSize:
    case eMuxBuiltinGetGlobalOffset:
    case eMuxBuiltinGetNumGroups:
    case eMuxBuiltinGetGroupId:
    case eMuxBuiltinGetEnqueuedLocalSize:
      ParamTys.push_back(Int32Ty);
      ParamNames.push_back("idx");
      LLVM_FALLTHROUGH;
    // Unranked Getters
    case eMuxBuiltinGetWorkDim:
    case eMuxBuiltinGetSubGroupId:
    case eMuxBuiltinGetNumSubGroups:
    case eMuxBuiltinGetMaxSubGroupSize:
    case eMuxBuiltinGetLocalLinearId:
    case eMuxBuiltinGetGlobalLinearId: {
      // Some builtins return uint, others return size_t
      RetTy = (ID == eMuxBuiltinGetWorkDim || ID == eMuxBuiltinGetSubGroupId ||
               ID == eMuxBuiltinGetNumSubGroups ||
               ID == eMuxBuiltinGetMaxSubGroupSize)
                  ? Int32Ty
                  : SizeTy;
      // All of our mux getters are readonly - they may never write data
      AB.addAttribute(Attribute::ReadOnly);
      break;
    }
    // Ranked Setters
    case eMuxBuiltinSetLocalId:
      ParamTys.push_back(Int32Ty);
      ParamNames.push_back("idx");
      LLVM_FALLTHROUGH;
    // Unranked Setters
    case eMuxBuiltinSetSubGroupId:
    case eMuxBuiltinSetNumSubGroups:
    case eMuxBuiltinSetMaxSubGroupSize: {
      RetTy = VoidTy;
      ParamTys.push_back(ID == eMuxBuiltinSetLocalId ? SizeTy : Int32Ty);
      ParamNames.push_back("val");
      break;
    }
    case eMuxBuiltinMemBarrier: {
      RetTy = VoidTy;
      for (auto PName : {"scope", "semantics"}) {
        ParamTys.push_back(Int32Ty);
        ParamNames.push_back(PName);
      }
      AB.addAttribute(Attribute::NoMerge);
      AB.addAttribute(Attribute::NoDuplicate);
      AB.addAttribute(Attribute::Convergent);
      break;
    }
    case eMuxBuiltinSubGroupBarrier:
    case eMuxBuiltinWorkGroupBarrier: {
      RetTy = VoidTy;
      for (auto PName : {"id", "scope", "semantics"}) {
        ParamTys.push_back(Int32Ty);
        ParamNames.push_back(PName);
      }
      AB.addAttribute(Attribute::NoMerge);
      AB.addAttribute(Attribute::NoDuplicate);
      AB.addAttribute(Attribute::Convergent);
      break;
    }
  }

  assert(RetTy);
  assert(ParamTys.size() == ParamNames.size());

  SmallVector<int, 4> SchedParamIdxs;
  // Fill up the scalar parameters with the default attributes.
  SmallVector<AttributeSet, 4> ParamAttrs(ParamTys.size(), AttributeSet());

  if (requiresSchedulingParameters(ID) &&
      getSchedulingParameterModuleMetadata(M)) {
    for (const auto &P : getMuxSchedulingParameters(M)) {
      ParamTys.push_back(P.ParamTy);
      ParamNames.push_back(P.ParamName);
      ParamAttrs.push_back(P.ParamAttrs);
      SchedParamIdxs.push_back(ParamTys.size() - 1);
    }
  }

  auto *const FnTy = FunctionType::get(RetTy, ParamTys, /*isVarArg*/ false);
  auto *const F = Function::Create(FnTy, Function::ExternalLinkage, FnName, &M);
  F->addFnAttrs(AB);

  // Add some extra attributes we know are always true.
  setDefaultBuiltinAttributes(*F);

  for (unsigned i = 0, e = ParamNames.size(); i != e; i++) {
    F->getArg(i)->setName(ParamNames[i]);
    auto AB = AttrBuilder(M.getContext(), ParamAttrs[i]);
    F->getArg(i)->addAttrs(AB);
  }

  setSchedulingParameterFunctionMetadata(*F, SchedParamIdxs);

  return F;
}

// By default we use two parameters:
// * one structure containing local work-group data
// * one structure containing non-local work-group data
SmallVector<BuiltinInfo::SchedParamInfo, 4>
BIMuxInfoConcept::getMuxSchedulingParameters(Module &M) {
  auto &Ctx = M.getContext();
  auto &DL = M.getDataLayout();
  AttributeSet DefaultAttrs;
  DefaultAttrs = DefaultAttrs.addAttribute(Ctx, Attribute::NonNull);
  DefaultAttrs = DefaultAttrs.addAttribute(Ctx, Attribute::NoAlias);

  BuiltinInfo::SchedParamInfo WIInfo;
  {
    auto *const WIInfoS = getWorkItemInfoStructTy(M);
    WIInfo.ID = SchedParamIndices::WI;
    WIInfo.ParamPointeeTy = WIInfoS;
    WIInfo.ParamTy = WIInfoS->getPointerTo();
    WIInfo.ParamName = "wi-info";
    WIInfo.ParamDebugName = WIInfoS->getStructName().str();
    WIInfo.PassedExternally = false;

    auto AB = AttrBuilder(Ctx, DefaultAttrs);
    AB.addAlignmentAttr(DL.getABITypeAlign(WIInfoS));
    AB.addDereferenceableAttr(DL.getTypeAllocSize(WIInfoS));
    WIInfo.ParamAttrs = AttributeSet::get(Ctx, AB);
  }

  BuiltinInfo::SchedParamInfo WGInfo;
  {
    auto *const WGInfoS = getWorkGroupInfoStructTy(M);
    WGInfo.ID = SchedParamIndices::WG;
    WGInfo.ParamPointeeTy = WGInfoS;
    WGInfo.ParamTy = WGInfoS->getPointerTo();
    WGInfo.ParamName = "wg-info";
    WGInfo.ParamDebugName = WGInfoS->getStructName().str();
    WGInfo.PassedExternally = true;

    auto AB = AttrBuilder(Ctx, DefaultAttrs);
    AB.addAlignmentAttr(DL.getABITypeAlign(WGInfoS));
    AB.addDereferenceableAttr(DL.getTypeAllocSize(WGInfoS));
    WGInfo.ParamAttrs = AttributeSet::get(Ctx, AB);
  }

  return {WIInfo, WGInfo};
}

SmallVector<BuiltinInfo::SchedParamInfo, 4>
BIMuxInfoConcept::getFunctionSchedulingParameters(Function &F) {
  // Query function metadata to determine whether this function has scheduling
  // parameters
  auto ParamIdxs = getSchedulingParameterFunctionMetadata(F);
  if (ParamIdxs.empty()) {
    return {};
  }

  auto SchedParamInfo = getMuxSchedulingParameters(*F.getParent());
  // We don't allow a function to have a subset of the global scheduling
  // parameters.
  assert(ParamIdxs.size() >= SchedParamInfo.size());
  // Set the concrete argument values on each of the scheduling parameter data.
  for (auto it : zip(SchedParamInfo, ParamIdxs)) {
    // Some scheduling parameters may not be present (returning an index of
    // -1), in which case skip their concrete argument values.
    if (std::get<1>(it) >= 0) {
      std::get<0>(it).ArgVal = F.getArg(std::get<1>(it));
    }
  }

  return SchedParamInfo;
};

Value *BIMuxInfoConcept::initializeSchedulingParamForWrappedKernel(
    const BuiltinInfo::SchedParamInfo &Info, IRBuilder<> &B, Function &IntoF,
    Function &) {
  // We only expect to have to initialize the work-item info. The work-group
  // info is straight passed through.
  (void)IntoF;
  assert(!Info.PassedExternally && Info.ID == SchedParamIndices::WI &&
         Info.ParamName == "wi-info" &&
         Info.ParamPointeeTy == getWorkItemInfoStructTy(*IntoF.getParent()));
  return B.CreateAlloca(Info.ParamPointeeTy,
                        /*ArraySize*/ nullptr, Info.ParamName);
}

}  // namespace utils
}  // namespace compiler
