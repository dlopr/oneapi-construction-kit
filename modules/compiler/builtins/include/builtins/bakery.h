// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Binary bakery.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BAKERY_BAKERY_H_INCLUDED
#define BAKERY_BAKERY_H_INCLUDED

#include <cstddef>
#include <cstdint>

#include "cargo/array_view.h"

namespace builtins {
/// @addtogroup builtins
/// @{

namespace file {
/// @brief Embedded file capabilities
enum capabilities : uint32_t {
  /// @brief Default (minimal) capabilities (64-bit).
  CAPS_DEFAULT = 0x0,
  /// @brief 32-bit file (default is 64-bit).
  CAPS_32BIT = 0x1,
  /// @brief File with floating point double types.
  CAPS_FP64 = 0x2,
  /// @brief File with floating point half types.
  CAPS_FP16 = 0x4
};

using capabilities_bitfield = uint32_t;
}  // namespace file

/// @brief Get the builtins header source.
///
/// @return Reference to the builtins header source.
cargo::array_view<const uint8_t> get_api_src_file();

/// @brief Get the builtins header source for OpenCL 3.0.
///
/// @return Reference to the builtins header source for OpenCL 3.0.
cargo::array_view<const uint8_t> get_api_30_src_file();

/// @brief Get the force-include header for a core device
///
/// @param[in] device_name Core device's device_name
///
/// @return Reference to the header, if available, or an empty file otherwise
cargo::array_view<const uint8_t> get_api_force_file_device(
    const char *const device_name);

/// @brief Get a builtins precompiled header based on the required capabilities.
///
/// @param[in] caps A capabilities_bitfield of the required capabilities.
///
/// @return Reference to the precompiled header.
cargo::array_view<const uint8_t> get_pch_file(file::capabilities_bitfield caps);

/// @brief Get a builtins bitcode file based on the required capabilities.
///
/// @param[in] caps A capabilities_bitfield of the required capabilities.
///
/// @return Reference to the bitcode file.
cargo::array_view<const uint8_t> get_bc_file(file::capabilities_bitfield caps);

/// @}
}  // namespace builtins

#endif  // BAKERY_BAKERY_H_INCLUDED
