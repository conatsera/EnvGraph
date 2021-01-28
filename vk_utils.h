#pragma once
#ifndef ENGINE_VK_UTILS_H
#define ENGINE_VK_UTILS_H

#include <vulkan/vulkan.hpp>

constexpr const uint64_t kFenceTimeout = 100000000;

constexpr const uint32_t kDefaultWidth = 1536;
constexpr const uint32_t kDefaultHeight = 768;

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

constexpr const vk::Format kDepthFormat = vk::Format::eD16Unorm;

}  // namespace Engine

#endif