// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T>
T half_exp10(T x) {
  const T log_e_base10 = (T)3.32192809489f;
  return __abacus_half_exp2(log_e_base10 * x);
}
}  // namespace

abacus_float ABACUS_API __abacus_half_exp10(abacus_float x) {
  return half_exp10<>(x);
}
abacus_float2 ABACUS_API __abacus_half_exp10(abacus_float2 x) {
  return half_exp10<>(x);
}
abacus_float3 ABACUS_API __abacus_half_exp10(abacus_float3 x) {
  return half_exp10<>(x);
}
abacus_float4 ABACUS_API __abacus_half_exp10(abacus_float4 x) {
  return half_exp10<>(x);
}
abacus_float8 ABACUS_API __abacus_half_exp10(abacus_float8 x) {
  return half_exp10<>(x);
}
abacus_float16 ABACUS_API __abacus_half_exp10(abacus_float16 x) {
  return half_exp10<>(x);
}
