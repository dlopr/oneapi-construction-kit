// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T>
T _(T x, T y) {
  return __abacus_half_exp2(y * __abacus_log2(x));
}
}  // namespace

#define DEF(TYPE) \
  TYPE ABACUS_API __abacus_half_powr(TYPE x, TYPE y) { return _(x, y); }

DEF(abacus_float)
DEF(abacus_float2)
DEF(abacus_float3)
DEF(abacus_float4)
DEF(abacus_float8)
DEF(abacus_float16)
