// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/command_buffer.h>
#include <vk/command_pool.h>
#include <vk/device.h>

namespace vk {
command_pool_t::command_pool_t(VkCommandPoolCreateFlags flags,
                               uint32_t queueFamilyIndex,
                               vk::allocator allocator)
    : command_buffers(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      flags(flags),
      queueFamilyIndex(queueFamilyIndex),
      allocator(allocator) {}

command_pool_t::~command_pool_t() {}

VkResult CreateCommandPool(vk::device device,
                           const VkCommandPoolCreateInfo *pCreateInfo,
                           vk::allocator allocator,
                           vk::command_pool *pCommandPool) {
  (void)device;
  vk::command_pool command_pool = allocator.create<vk::command_pool_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, pCreateInfo->flags,
      pCreateInfo->queueFamilyIndex, allocator);

  if (!command_pool) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pCommandPool = command_pool;
  return VK_SUCCESS;
}

void DestroyCommandPool(vk::device device, vk::command_pool commandPool,
                        vk::allocator allocator) {
  if (commandPool == VK_NULL_HANDLE) {
    return;
  }

  if (!commandPool->command_buffers.empty()) {
    FreeCommandBuffers(device, commandPool, commandPool->command_buffers.size(),
                       commandPool->command_buffers.data());
  }
  allocator.destroy(commandPool);
}

VkResult ResetCommandPool(vk::device device, vk::command_pool commandPool,
                          VkCommandPoolResetFlags flags) {
  (void)flags;
  mux_result_t error;
  for (vk::command_buffer command_buffer : commandPool->command_buffers) {
    command_buffer->descriptor_sets.clear();

    for (auto &barrier_info : command_buffer->barrier_group_infos) {
      muxDestroyCommandBuffer(device->mux_device, barrier_info->command_buffer,
                              commandPool->allocator.getMuxAllocator());
      muxDestroySemaphore(device->mux_device, barrier_info->semaphore,
                          commandPool->allocator.getMuxAllocator());
    }

    command_buffer->barrier_group_infos.clear();

    if ((error = muxResetCommandBuffer(command_buffer->main_command_buffer))) {
      return vk::getVkResult(error);
    }

    if ((error = muxResetSemaphore(command_buffer->main_semaphore))) {
      return vk::getVkResult(error);
    }

    command_buffer->error = VK_SUCCESS;
    command_buffer->state = command_buffer_t::initial;
  }
  return VK_SUCCESS;
}
}  // namespace vk
