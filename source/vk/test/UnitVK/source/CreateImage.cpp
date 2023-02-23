// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateImage

class CreateImage : public uvk::DeviceTest {
 public:
  CreateImage() : createInfo(), image(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
  }

  virtual void TearDown() override {
    if (image) {
      vkDestroyImage(device, image, nullptr);
    }

    DeviceTest::TearDown();
  }

  VkImageCreateInfo createInfo;
  VkImage image;
};

TEST_F(CreateImage, DISABLED_Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateImage(device, &createInfo, nullptr, &image));
}

TEST_F(CreateImage, DISABLED_DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateImage(device, &createInfo,
                                             uvk::defaultAllocator(), &image));
  vkDestroyImage(device, image, uvk::defaultAllocator());
  image = VK_NULL_HANDLE;
}

TEST_F(CreateImage, DISABLED_ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateImage(device, &createInfo, uvk::nullAllocator(), &image));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
