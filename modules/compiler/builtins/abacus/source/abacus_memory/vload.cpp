// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_memory.h>

namespace {
template <typename T, typename U>
T vload(const size_t offset, U p) {
  const U pPlusOffset = p + (offset * TypeTraits<T>::num_elements);

  T result = (T)0;
  for (unsigned int i = 0; i < TypeTraits<T>::num_elements; i++) {
    result[i] = pPlusOffset[i];
  }

  return result;
}
}  // namespace

#define DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, ADDRESS_SPACE) \
  TYPE##SIZE ABACUS_API __abacus_vload##SIZE(size_t offset,                 \
                                             const TYPE ADDRESS_SPACE* x) { \
    return vload<TYPE##SIZE>(offset, x);                                    \
  }

#ifdef __OPENCL_VERSION__
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE)                         \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, __global)   \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, __local)    \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, __constant) \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, )
#else
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE) \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, )
#endif

#define DEF(TYPE)                 \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 2) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 3) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 4) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 8) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 16)

DEF(abacus_char);
DEF(abacus_uchar);
DEF(abacus_short);
DEF(abacus_ushort);
DEF(abacus_int);
DEF(abacus_uint);
DEF(abacus_long);
DEF(abacus_ulong);
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half);
#endif  // __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_float);
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
