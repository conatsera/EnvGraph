#pragma once
#ifndef GRAPHICS_ENGINE_DEFS_H
#define GRAPHICS_ENGINE_DEFS_H

#include <cinttypes>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace Engine
{

const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

typedef struct _MemoryBarrierResource
{
    vk::AccessFlagBits m_src;
    vk::AccessFlagBits m_dst;
    _MemoryBarrierResource(vk::AccessFlagBits src, vk::AccessFlagBits dst) : m_src(src), m_dst(dst)
    {
    }
} MemoryBarrierResource;

typedef struct _BufferBarrierResource : public MemoryBarrierResource
{
    vk::Buffer &m_buffer;
    vk::DescriptorBufferInfo m_descBufferInfo;
    _BufferBarrierResource(vk::AccessFlagBits src, vk::AccessFlagBits dst, vk::Buffer &buffer,
                           vk::DescriptorBufferInfo descBufferInfo)
        : MemoryBarrierResource(src, dst), m_buffer(buffer), m_descBufferInfo(descBufferInfo)
    {
    }
} BufferBarrierResource;

typedef struct _ImageBarrierResource : public MemoryBarrierResource
{
    vk::Image &m_image;
    vk::ImageSubresourceRange m_imageSubresourceRange;
    _ImageBarrierResource(vk::AccessFlagBits src, vk::AccessFlagBits dst, vk::Image &image,
                          vk::ImageSubresourceRange imageSubresourceRange)
        : MemoryBarrierResource(src, dst), m_image(image), m_imageSubresourceRange(imageSubresourceRange)
    {
    }
} ImageBarrierResource;

typedef struct EngineBuffer
{
    vk::Buffer buffer;
    vk::DeviceMemory devMem;
} EngineBuffer_t;

enum class EngineTextureType : uint8_t
{
    SOLID_COLOR,
    GRADIENT,
    IMAGE
};

typedef struct
{
    glm::vec2 u;
    glm::vec2 v;
} EngineTextureCoords;

typedef struct EngineTexture
{
    EngineTextureType type;
    glm::vec3 colorA;
    glm::vec3 colorB;
    vk::Image image;

    EngineTextureCoords coords;
} EngineTexture_t;

typedef uint32_t DescriptorSetID;

constexpr const DescriptorSetID INVALID_DESCRIPTOR_SET_ID = UINT32_MAX;

} // namespace Engine

#endif