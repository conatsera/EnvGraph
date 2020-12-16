#pragma once
#ifndef ENGINE_VK_UTILS_H
#define ENGINE_VK_UTILS_H

#include <vulkan/vulkan.hpp>

namespace Engine {


template <typename TargetType, typename SourceType>
VULKAN_HPP_INLINE TargetType checked_cast(SourceType value) {
  static_assert(sizeof(TargetType) <= sizeof(SourceType),
                "No need to cast from smaller to larger type!");
  static_assert(!std::numeric_limits<TargetType>::is_signed,
                "Only unsigned types supported!");
  static_assert(!std::numeric_limits<SourceType>::is_signed,
                "Only unsigned types supported!");
  assert(value <= std::numeric_limits<TargetType>::max());
  return static_cast<TargetType>(value);
};

vk::UniqueInstance createInstance(
    std::string const& appName, std::string const& engineName,
    vk::InstanceCreateInfo& createdCreateInfo,
    std::vector<std::string> const& layers = {},
    std::vector<std::string> const& extensions = {},
    uint32_t apiVersion = VK_API_VERSION_1_1);

vk::UniqueDebugUtilsMessengerEXT createDebugUtilsMessenger(
    vk::UniqueInstance& instance);

const constexpr vk::ImageCreateInfo depthStencilImageCreateInfo{
    vk::ImageCreateFlags(),
    vk::ImageType::e2D,
    vk::Format::eD16Unorm,
    vk::Extent3D{1440, 1600, 1},
    1,
    1,
    vk::SampleCountFlagBits::e1,
    vk::ImageTiling::eOptimal,
    vk::ImageUsageFlagBits::eDepthStencilAttachment};

}  // namespace Engine

#endif