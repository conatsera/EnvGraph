#pragma once
#ifndef GRAPHICS_PIPELINES_UI_H
#define GRAPHICS_PIPELINES_UI_H

#include <filesystem>
#include <map>

#include <vulkan/vulkan.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../vk_utils.h"

#include "../pipelines.h"

#include "../ui/items.h"

namespace Engine
{
namespace UI
{

enum UIBufferIDs : Pipelines::BufferID_t
{
    kUIFontBufferID = 0,
    kCubeBufferID = 1
};

enum UIImageIDs : Pipelines::ImageID_t
{
    kFontImage = 0
};

enum UISamplerIDs : Pipelines::SamplerID_t
{
    kFontSampler = 0
};

struct UIGlyph
{
    uint16_t widechar;

    glm::vec2 x;
    glm::vec2 y;
    glm::vec2 u;
    glm::vec2 v;
    float adv_x;
};

class UIFont
{
  public:
    UIFont(){};
    ~UIFont();

    void AddGlyph(uint16_t c, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                  float advance_x)
    {
        glyphs.insert_or_assign(c, UIGlyph{c, {x0, x1}, {y0, y1}, {u0, u1}, {v0, v1}, advance_x});
    }

    std::byte *GetGlyphTexture() const
    {
        return glyphTexture;
    }
    vk::Extent2D GetGlyphTextureExtents() const
    {
        return glyphTextureExtents;
    }
    UIGlyph GetGlyph(uint16_t codepoint) const
    {
        return glyphs.at(codepoint);
    }
    glm::vec2 GetUVScale() const
    {
        return m_uvScale;
    }
    glm::vec2 GetSolidRect() const {
        return m_solidRect;
    }

    void SetupFont(std::filesystem::path fontpath);

  private:
    std::map<uint16_t, UIGlyph> glyphs;
    std::byte *glyphTexture;
    vk::Extent2D glyphTextureExtents;
    glm::vec2 m_uvScale;
    glm::vec2 m_solidRect;
};

class UIPipeline : public Pipelines::EnginePipeline<1, 1, 2, 1, 1, 0, 1>
{
  public:
    virtual void Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass) override;

    void SetupShaders(const vk::UniqueDevice &device)
    {
        SetupVertexBuffers(device);
        SetupFragmentFontTexture(device);
    };

    virtual void Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer,
                        vk::Extent2D windowExtents) override;

    virtual void Resized(vk::Extent2D windowExtents) override;

    const LabelID AddLabel(Label label);
    void RemoveLabel(LabelID labelId);

    const ButtonID AddButton(Button button);
    void RemoveButton(ButtonID buttonId);

  private:
    void CreateShaders(const vk::UniqueDevice &device);

    void SetupVertexBuffers(const vk::UniqueDevice &device);
    void SetupFragmentFontTexture(const vk::UniqueDevice &device);

  private:
    UIFont *m_font;
    vk::Sampler m_fontSampler;
    vk::ImageView m_fontImageView;

    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

    vk::UniqueShaderModule m_uiVertShaderModule;
    vk::UniqueShaderModule m_uiFragShaderModule;

    vk::Extent2D m_savedWindowExtents{kDefaultWidth, kDefaultHeight};

    uint32_t m_labelVertexCount = 0;
    EngineBuffer_t m_labelEngBuffer;
    UIVertex<2> *m_labelVertexBuffer;
    std::map<LabelID, Label> m_labels;

    uint32_t m_buttonVertexCount = 0;
    EngineBuffer_t m_buttonEngBuffer;
    UIVertex<2> *m_buttonVertexBuffer;
    std::map<ButtonID, Button> m_buttons;
};

} // namespace UI
} // namespace Engine

#endif