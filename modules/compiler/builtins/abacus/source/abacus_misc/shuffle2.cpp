// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_misc.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
struct SizeLaundry {
  static const unsigned num_elements;
};

template <typename T>
const unsigned SizeLaundry<T>::num_elements = TypeTraits<T>::num_elements;

template <typename R, typename T, typename M>
R shuffle2(const T x, const T y, const M m) {
  typedef typename TypeTraits<M>::ElementType MElementType;
  const MElementType num_elements =
      abacus::detail::cast::convert<MElementType>(SizeLaundry<T>::num_elements);
  const M maskedM = m % (num_elements * 2);
  const M furtherMaskedM = m % num_elements;

  const typename TypeTraits<M>::SignedType inSecond = (maskedM >= num_elements);

  R r{};

  for (unsigned int i = 0; i < SizeLaundry<M>::num_elements; i++) {
    r[i] = inSecond[i] ? y[furtherMaskedM[i]] : x[furtherMaskedM[i]];
  }

  return r;
}
}  // namespace

#define DEF_WITH_BOTH_SIZES(TYPE, IN_SIZE, OUT_SIZE) \
  TYPE##OUT_SIZE __abacus_shuffle2(                  \
      TYPE##IN_SIZE x, TYPE##IN_SIZE y,              \
      TypeTraits<TYPE##OUT_SIZE>::UnsignedType m) {  \
    return shuffle2<TYPE##OUT_SIZE>(x, y, m);        \
  }

#define DEF_WITH_SIZE(TYPE, SIZE)    \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 2) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 3) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 4) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 8) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 16)

#define DEF(TYPE)        \
  DEF_WITH_SIZE(TYPE, 2) \
  DEF_WITH_SIZE(TYPE, 3) \
  DEF_WITH_SIZE(TYPE, 4) \
  DEF_WITH_SIZE(TYPE, 8) \
  DEF_WITH_SIZE(TYPE, 16)

DEF(abacus_char);
DEF(abacus_uchar);
DEF(abacus_short);
DEF(abacus_ushort);
DEF(abacus_int);
DEF(abacus_uint);
DEF(abacus_long);
DEF(abacus_ulong);
DEF(abacus_float);
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
