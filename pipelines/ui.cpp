#include "ui.h"

#include <filesystem>
#include <fstream>
#include <iostream>

//#include <embedder.h>

#include "../engine.h"
#include "../engine_defs.h"

#include "../ui/prims.h"

namespace Engine
{
namespace UI
{

void UIPipeline::Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass)
{
    EnginePipeline::Setup(device, phyDevMemProps, graphicsQueueFamilyIndex, computeQueueFamilyIndex,
                          graphicsQueueStartIndex, computeQueueStartIndex, renderPass);

    m_font = new UIFont();
/*
    FlutterRendererConfig config{};
    config.type = FlutterRendererType::kOpenGL;
    config.open_gl.struct_size = sizeof(config.open_gl);
    config.open_gl.make_current = [](void* ) -> bool {
        return true;  
    };
    config.open_gl.clear_current = [](void* ) -> bool {
        return true;  
    };
    config.open_gl.present = [](void* userdata) -> bool {
        return true;
    };
    config.open_gl.fbo_callback = [](void*) -> uint32_t {
        return 0;
    };

    std::string assets_path = "./flutter_assets";
    std::string icudtl_path = "./flutter_assets/icudtl.dat";
    FlutterProjectArgs args = {
      .struct_size = sizeof(FlutterProjectArgs),
      .assets_path = assets_path.c_str(),
      .icu_data_path =
          icudtl_path.c_str(),  // Find this in your bin/cache directory.
    };
    FlutterEngine engine = nullptr;
    FlutterEngineResult result =
        FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config,  // renderer
                        &args, device.get(), &engine);
    assert(result == kSuccess && engine != nullptr);
*/

    m_pushConstantRanges[0] = {vk::ShaderStageFlagBits::eVertex, 0, sizeof(float) * 4};

    SetupShaders(device);
    SetupPipeline(device);

    CreateShaders(device);

    // TODO: create second vertex input binding for 3D
    vertexInputBindingDesc.stride = sizeof(UIVertex<2>);

    vertexInputAttrDesc = {{0, 0, vk::Format::eR32G32Sfloat, offsetof(UIVertex<2>, pos)},
                           {1, 0, vk::Format::eR32G32Sfloat, offsetof(UIVertex<2>, uv)},
                           {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(UIVertex<2>, color)}};

    vertexInfoStateCI.setVertexAttributeDescriptions(vertexInputAttrDesc);

    rasterStateCI.cullMode = vk::CullModeFlagBits::eNone;
    rasterStateCI.frontFace = vk::FrontFace::eCounterClockwise;

    colorBlendAttachmentState.blendEnable = VK_TRUE;
    colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    depthStencilStateCI = vk::PipelineDepthStencilStateCreateInfo{};

    vk::GraphicsPipelineCreateInfo graphicsPipelineCI{vk::PipelineCreateFlags{},
                                                      m_shaderStages,
                                                      &vertexInfoStateCI,
                                                      &inputAssemblyStateCI,
                                                      nullptr,
                                                      &viewportStateCI,
                                                      &rasterStateCI,
                                                      &multisampleStateCI,
                                                      &depthStencilStateCI,
                                                      &colorBlendStateCI,
                                                      &dynamicStateCI,
                                                      m_pipelineLayout.get(),
                                                      renderPass.get(),
                                                      0,
                                                      nullptr,
                                                      0};

    m_pipeline = device->createGraphicsPipelineUnique(m_pipelineCache.get(), graphicsPipelineCI).value;
}

void UIPipeline::CreateShaders(const vk::UniqueDevice &device)
{
    std::vector<uint32_t> uiVertSpriv;

    vk::PipelineShaderStageCreateInfo uiVertPipelineShaderStageCI{vk::PipelineShaderStageCreateFlags{},
                                                                  vk::ShaderStageFlagBits::eVertex, nullptr, "main"};
#include "ui.vert.h"
    vk::ShaderModuleCreateInfo uiVertShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(ui_vert), ui_vert};

    m_uiVertShaderModule = device->createShaderModuleUnique(uiVertShaderModuleCI);

    uiVertPipelineShaderStageCI.setModule(m_uiVertShaderModule.get());

    m_shaderStages.push_back(uiVertPipelineShaderStageCI);

    std::vector<uint32_t> uiFragSpriv;

    vk::PipelineShaderStageCreateInfo uiFragPipelineShaderStageCI{vk::PipelineShaderStageCreateFlags{},
                                                                  vk::ShaderStageFlagBits::eFragment, nullptr, "main"};
#include "ui.frag.h"
    vk::ShaderModuleCreateInfo uiFragShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(ui_frag), ui_frag};

    m_uiFragShaderModule = device->createShaderModuleUnique(uiFragShaderModuleCI);

    uiFragPipelineShaderStageCI.setModule(m_uiFragShaderModule.get());

    m_shaderStages.push_back(uiFragPipelineShaderStageCI);
}

void UIPipeline::SetupVertexBuffers(const vk::UniqueDevice &device)
{
    vk::BufferCreateInfo vertBufferCI{vk::BufferCreateFlags{}, kInitialLabelVertexBufferSize,
                                      vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive};

    m_labelEngBuffer =
        CreateNewBuffer(device, kCubeBufferID, vertBufferCI, reinterpret_cast<void **>(&m_labelVertexBuffer),
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    device->bindBufferMemory(m_labelEngBuffer.buffer, m_labelEngBuffer.devMem, 0);

    m_buttonEngBuffer =
        CreateNewBuffer(device, kCubeBufferID, vertBufferCI, reinterpret_cast<void **>(&m_buttonVertexBuffer),
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    device->bindBufferMemory(m_buttonEngBuffer.buffer, m_buttonEngBuffer.devMem, 0);
}
#define IM_COL32_R_SHIFT 0
#define IM_COL32_G_SHIFT 8
#define IM_COL32_B_SHIFT 16
#define IM_COL32_A_SHIFT 24
#define IM_COL32(R, G, B, A)                                                                                           \
    (((uint32_t)(A) << IM_COL32_A_SHIFT) | ((uint32_t)(B) << IM_COL32_B_SHIFT) | ((uint32_t)(G) << IM_COL32_G_SHIFT) | \
     ((uint32_t)(R) << IM_COL32_R_SHIFT))

void UIPipeline::SetupFragmentFontTexture(const vk::UniqueDevice &device)
{
    std::filesystem::path fontpath = "../extern/plex/IBM-Plex-Sans/fonts/complete/ttf/IBMPlexSans-Regular.ttf";

    vk::SamplerCreateInfo fontSamplerCI{vk::SamplerCreateFlags(),
                                        vk::Filter::eLinear,
                                        vk::Filter::eLinear,
                                        vk::SamplerMipmapMode::eLinear,
                                        vk::SamplerAddressMode::eRepeat,
                                        vk::SamplerAddressMode::eRepeat,
                                        vk::SamplerAddressMode::eRepeat,
                                        0,
                                        VK_FALSE,
                                        1.0f,
                                        VK_FALSE,
                                        vk::CompareOp::eNever,
                                        -1000,
                                        1000};

    m_fontSampler = CreateNewSampler(device, kFontSampler, fontSamplerCI);

    vk::DescriptorSetLayoutBinding textureLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler,
                                                        vk::ShaderStageFlagBits::eFragment, m_fontSampler};

    std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings{textureLayoutBinding};

    vk::DescriptorSetLayoutCreateInfo fontSamplerDescSetLayoutCreateInfo{vk::DescriptorSetLayoutCreateFlags(),
                                                                         layoutBindings};

    DescriptorSetID fontDescSetID = AddDescriptorLayoutSets(device, fontSamplerDescSetLayoutCreateInfo);

    m_font->SetupFont(fontpath);

    std::byte *glyphTex = m_font->GetGlyphTexture();
    vk::Extent2D glyphTextExtents = m_font->GetGlyphTextureExtents();
    const size_t pixelCount = glyphTextExtents.width * glyphTextExtents.height;
    uint32_t *glyphTexRGBA32 = (uint32_t *)malloc(pixelCount * 4);

    const uint8_t *src = (const uint8_t *)glyphTex;
    uint32_t *dst = glyphTexRGBA32;
    for (int n = pixelCount; n > 0; n--)
        *dst++ = IM_COL32(255, 255, 255, (uint32_t)(*src++));

    vk::ImageCreateInfo imageCI{vk::ImageCreateFlags(),
                                vk::ImageType::e2D,
                                vk::Format::eR8G8B8A8Unorm,
                                vk::Extent3D{glyphTextExtents.width, glyphTextExtents.height, 1},
                                1,
                                1,
                                vk::SampleCountFlagBits::e1,
                                vk::ImageTiling::eOptimal,
                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                                vk::SharingMode::eExclusive,
                                {},
                                vk::ImageLayout::eUndefined};

    vk::ImageSubresourceRange imageSR{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    vk::ImageLayout finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    m_fontImageView =
        CreateNewImage(device, kFontImage, 0, fontDescSetID, imageCI, imageSR, finalLayout, m_fontSampler);

    const size_t fontTextureBufferSize = glyphTextExtents.width * glyphTextExtents.height * 4 * sizeof(std::byte);

    vk::BufferCreateInfo textureBufferCI{vk::BufferCreateFlags(), fontTextureBufferSize,
                                         vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive};

    std::byte *fontTextureBuffer = nullptr;
    EngineBuffer_t fontTextureEngBuffer =
        CreateNewBuffer(device, kUIFontBufferID, textureBufferCI, reinterpret_cast<void **>(&fontTextureBuffer),
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (fontTextureBuffer != nullptr)
        memcpy(fontTextureBuffer, glyphTexRGBA32, fontTextureBufferSize);

    device->bindBufferMemory(fontTextureEngBuffer.buffer, fontTextureEngBuffer.devMem, 0);

    vk::MappedMemoryRange fontTextureMemRange{fontTextureEngBuffer.devMem, 0, fontTextureBufferSize};

    device->flushMappedMemoryRanges(fontTextureMemRange);
    device->unmapMemory(fontTextureEngBuffer.devMem);

    vk::ImageMemoryBarrier copyBarrier{vk::AccessFlags{},           vk::AccessFlagBits::eTransferWrite,
                                       vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                       VK_QUEUE_FAMILY_IGNORED,     VK_QUEUE_FAMILY_IGNORED,
                                       m_images[kFontImage],        imageSR};

    vk::CommandBufferAllocateInfo graphicsCommandBufferAI{m_graphicsCommandPool.get(), vk::CommandBufferLevel::ePrimary,
                                                          1};

    vk::UniqueCommandBuffer cmdBuffer;
    cmdBuffer.swap(device->allocateCommandBuffersUnique(graphicsCommandBufferAI)[0]);

    vk::CommandBufferBeginInfo cmdBufBI{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    cmdBuffer->begin(cmdBufBI);

    cmdBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                               copyBarrier);

    vk::BufferImageCopy copyRegion = {};
    copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent.depth = 1;
    copyRegion.imageExtent.height = glyphTextExtents.height;
    copyRegion.imageExtent.width = glyphTextExtents.width;

    cmdBuffer->copyBufferToImage(fontTextureEngBuffer.buffer, m_images[kFontImage],
                                 vk::ImageLayout::eTransferDstOptimal, copyRegion);

    vk::ImageMemoryBarrier useBarrier{vk::AccessFlagBits::eTransferWrite,
                                      vk::AccessFlagBits::eShaderRead,
                                      vk::ImageLayout::eTransferDstOptimal,
                                      vk::ImageLayout::eShaderReadOnlyOptimal,
                                      VK_QUEUE_FAMILY_IGNORED,
                                      VK_QUEUE_FAMILY_IGNORED,
                                      m_images[kFontImage],
                                      imageSR};

    cmdBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {},
                               {}, useBarrier);

    vk::SubmitInfo samplerSubmitInfo{{}, {}, cmdBuffer.get(), {}};

    cmdBuffer->end();

    m_graphicsQueues[0].submit(samplerSubmitInfo);

    device->waitIdle();
}

void UIPipeline::Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer, vk::Extent2D windowExtents)
{
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_descriptorSets,
                                 nullptr);

    constexpr const vk::DeviceSize offsets[1]{0};
    cmdBuffer.bindVertexBuffers(0, 1, &m_labelEngBuffer.buffer, offsets);

    const float scale[2]{ 2.0f , 2.0f};
    const float translate[2]{ -1.f, -1.f};

    cmdBuffer.pushConstants(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(float) * 2, &scale);
    cmdBuffer.pushConstants(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, sizeof(float) * 2,
                            sizeof(float) * 2, &translate);

    cmdBuffer.draw(m_labelVertexCount, 1, 0, 0);

    cmdBuffer.bindVertexBuffers(0, 1, &m_buttonEngBuffer.buffer, offsets);

    cmdBuffer.draw(m_buttonVertexCount, 1, 0, 0);
}

inline void CalculateLabelAlignmentAdjustment(Label label, UIFont *&font, float &alignmentAdjustment)
{
    switch (label.alignment)
    {
    case TextAlign::CENTER:
    {
        for (size_t c = 0; c < label.text.length(); c++)
        {
            auto glyph = font->GetGlyph(label.text.at(c));

            alignmentAdjustment += (((glyph.x[1] - glyph.x[0]) * label.scale.x) / 2048);
        }
        alignmentAdjustment /= -2;
        break;
    }
    default:
        break;
    }

}

inline void CreateLabelChar(Label label, float &currentXPos, size_t c, float alignmentAdjustment,
                            vk::Extent2D windowExtents, UIFont *&font,
                            UIVertex<2> *&labelVertexBuffer, size_t bufferIndex)
{
    auto glyph = font->GetGlyph(label.text.at(c));

    float xPos = currentXPos;
    glm::vec2 x{
        label.position.x + ((glyph.x[0] * label.scale.x) / 2048) + alignmentAdjustment,
        label.position.x + ((glyph.x[1] * label.scale.x) / 2048) + alignmentAdjustment
    };
    glm::vec2 y{
        label.position.y + ((glyph.y[0] * label.scale.y) / (2048 / ((float)windowExtents.width / (float)windowExtents.height))),
        label.position.y + ((glyph.y[1] * label.scale.y) / (2048 / ((float)windowExtents.width / (float)windowExtents.height)))
    };
    currentXPos += (x[1] - x[0]);
    x[0] += xPos;
    x[1] += xPos;

    auto color = glm::vec<4, uint8_t>(glm::vec3{label.texture.colorA * 255.f}, 255);
    UIVertex<2> *labelVertexStart = &labelVertexBuffer[bufferIndex];

    CreateQuad<UIVertex<2>>(labelVertexStart, x, y, glyph.u, glyph.v, color);
}

void UIPipeline::Resized(vk::Extent2D windowExtents)
{
    m_savedWindowExtents = windowExtents;

    for (auto &label: m_labels)
    {
        float alignmentAdjustment = 0;
        CalculateLabelAlignmentAdjustment(label.second, m_font, alignmentAdjustment);
        
        float currentXPos = 0;
            for (size_t c = 0; c < label.second.text.length(); c++)
        {
            const size_t bufferIndex = label.second.startVertex + (c * 6);

            CreateLabelChar(label.second, currentXPos, c, alignmentAdjustment, windowExtents, m_font, m_labelVertexBuffer, bufferIndex);
        }
    }
}

const LabelID UIPipeline::AddLabel(Label label)
{
    LabelID labelId = kInvalidLabelID;

    float alignmentAdjustment = 0;

    CalculateLabelAlignmentAdjustment(label, m_font, alignmentAdjustment);

    switch (label.texture.type)
    {
    case (EngineTextureType::SOLID_COLOR): {
        float currentXPos = 0;
        label.startVertex = m_labelVertexCount;
        for (size_t c = 0; c < label.text.length(); c++)
        {
            const size_t bufferIndex = m_labelVertexCount;

            CreateLabelChar(label, currentXPos, c, alignmentAdjustment,  m_savedWindowExtents, m_font, m_labelVertexBuffer, bufferIndex);

            m_labelVertexCount += 6;
        }
        label.endVertex = m_labelVertexCount - 1;
        labelId = std::rand();
        m_labels.try_emplace(labelId, label);
        break;
    }
    case (EngineTextureType::GRADIENT): {
        break;
    }
    case (EngineTextureType::IMAGE): {
        break;
    }
    }

    return labelId;
}

void UIPipeline::RemoveLabel(LabelID labelId)
{
    m_labels.erase(labelId);

    auto lastVertexCount = m_labelVertexCount;
    m_labelVertexCount = 0;

    for (auto label : m_labels)
        AddLabel(label.second);

    for (int v = m_labelVertexCount; v < lastVertexCount; v++)
        m_labelVertexBuffer[v] = UIVertex<2>{};
}

const ButtonID UIPipeline::AddButton(Button button)
{
    ButtonID buttonId = kInvalidButtonID;
    switch (button.texture.type)
    {
    case (EngineTextureType::SOLID_COLOR): {
        button.startVertex = m_buttonVertexCount;
        glm::vec2 x{
            button.position.x,
            button.position.x + button.scale.x
        };
        glm::vec2 y{
            button.position.y,
            button.position.y + button.scale.y
        };

        auto color = glm::vec<4, uint8_t>(glm::vec3{button.texture.colorA * 255.f}, 255);

        auto solidRect = m_font->GetSolidRect();

        glm::vec2 u{
            solidRect.s,
            solidRect.t
        };
        glm::vec2 v{
            solidRect.s,
            solidRect.t
        };

        CreateQuad(m_buttonVertexBuffer, x, y, u, v, color);

        m_buttonVertexCount += 6;
        
        button.endVertex = m_buttonVertexCount - 1;
        buttonId = std::rand();
        m_buttons.try_emplace(buttonId, button);
        break;
    }
    case (EngineTextureType::GRADIENT): {
        break;
    }
    case (EngineTextureType::IMAGE): {
        break;
    }
    }

    return buttonId;
}

void UIPipeline::RemoveButton(ButtonID buttonId)
{
    m_buttons.erase(buttonId);

    auto lastVertexCount = m_buttonVertexCount;
    m_buttonVertexCount = 0;

    for (auto button : m_buttons)
        AddButton(button.second);

    for (int v = m_buttonVertexCount; v < lastVertexCount; v++)
        m_buttonVertexBuffer[v] = UIVertex<2>{};
}

void UIFont::SetupFont(std::filesystem::path fontpath)
{
    std::ifstream fontIfStream(fontpath, std::ios::in | std::ios::binary);

    const auto fontDataSize = std::filesystem::file_size(fontpath);

    char result[fontDataSize];

    if (std::filesystem::exists(fontpath))
    {
        fontIfStream.read(result, fontDataSize);

        const float fontSize = 32.0f;
        const uint16_t codepointRanges[]{
            0x0020,
            0x00FF, // Basic Latin + Latin Supplement
            0,
        };

        if (stbtt__isfont((stbtt_uint8 *)result))
        {
            stbtt_fontinfo plexFontInfo;

            if (!stbtt_InitFont(&plexFontInfo, (unsigned char *)result, 0))
                std::cerr << "Failed to initialize font" << std::endl;

            int glyphCount = 0;
            auto codepointsFound = std::vector<int>(codepointRanges[1] - codepointRanges[0]);

            for (unsigned int codepoint = codepointRanges[0]; codepoint <= codepointRanges[1]; codepoint++)
            {
                if (!stbtt_FindGlyphIndex(&plexFontInfo, codepoint))
                    continue;

                codepointsFound[glyphCount++] = codepoint;
            }

            auto glyphRect = std::vector<stbrp_rect>(glyphCount);
            auto glyphPackedChar = std::vector<stbtt_packedchar>(glyphCount);
            stbtt_pack_range glyphPackRange;

            glyphPackRange.font_size = fontSize;
            glyphPackRange.first_unicode_codepoint_in_range = 0;
            glyphPackRange.array_of_unicode_codepoints = codepointsFound.data();
            glyphPackRange.num_chars = glyphCount;
            glyphPackRange.chardata_for_range = glyphPackedChar.data();
            glyphPackRange.h_oversample = 3;
            glyphPackRange.v_oversample = 1;

            const float scale = stbtt_ScaleForPixelHeight(&plexFontInfo, fontSize);
            const int padding = 1;

            int surfaceArea = 0;
            for (int glyph_i = 0; glyph_i < glyphCount; glyph_i++)
            {
                int x0, y0, x1, y1;
                const int glyph_index_in_font = stbtt_FindGlyphIndex(&plexFontInfo, codepointsFound[glyph_i]);
                assert(glyph_index_in_font != 0);
                stbtt_GetGlyphBitmapBoxSubpixel(&plexFontInfo, glyph_index_in_font, scale * glyphPackRange.h_oversample,
                                                scale * glyphPackRange.v_oversample, 0, 0, &x0, &y0, &x1, &y1);
                glyphRect[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + glyphPackRange.h_oversample - 1);
                glyphRect[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + glyphPackRange.v_oversample - 1);
                surfaceArea += glyphRect[glyph_i].w * glyphRect[glyph_i].h;
            }

            const int surface_sqrt = (int)std::sqrt((double)surfaceArea) + 1;
            int fontTextureHeight = 0;
            int fontTextureWidth =
                (surface_sqrt >= 4096 * 0.7f)
                    ? 4096
                    : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;

            // 5. Start packing
            const int TEX_HEIGHT_MAX = 1024 * 32;
            stbtt_pack_context spc = {};
            stbtt_PackBegin(&spc, NULL, fontTextureWidth, TEX_HEIGHT_MAX, 0, 1, NULL);

            stbrp_rect solidRect;
            solidRect.w = 2;
            solidRect.h = 2;
            stbrp_pack_rects((stbrp_context *)spc.pack_info, &solidRect, 1);
            fontTextureHeight = std::max(fontTextureHeight, solidRect.y + solidRect.h);

            // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture
            // at this point.

            stbrp_pack_rects((stbrp_context *)spc.pack_info, glyphRect.data(), glyphCount);

            // Extend texture height and mark missing glyphs as non-packed so we won't render them.
            // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single
            // if larger than TexWidth?)
            for (int glyph_i = 0; glyph_i < glyphCount; glyph_i++)
                if (glyphRect[glyph_i].was_packed)
                    fontTextureHeight = std::max(fontTextureHeight, glyphRect[glyph_i].y + glyphRect[glyph_i].h);

            // 7. Allocate texture
            fontTextureHeight = std::pow(2, (int)std::log2((double)fontTextureHeight) + 1);
            m_uvScale = glm::vec2(1.0f / fontTextureWidth, 1.0f / fontTextureHeight);
            glyphTexture = (std::byte *)malloc(fontTextureWidth * fontTextureHeight);
            memset(glyphTexture, 0, fontTextureWidth * fontTextureHeight);
            spc.pixels = (unsigned char *)glyphTexture;
            spc.height = fontTextureHeight;
            spc.width = fontTextureWidth;

            m_solidRect = {(solidRect.x + 0.5f) * m_uvScale.x, (solidRect.y + 0.5f) * m_uvScale.x};

            glyphTextureExtents.width = fontTextureWidth;
            glyphTextureExtents.height = fontTextureHeight;

            // 8. Render/rasterize font characters into the texture
            stbtt_PackFontRangesRenderIntoRects(&spc, &plexFontInfo, &glyphPackRange, 1, glyphRect.data());

            // Apply multiply operator
            /*if (cfg.RasterizerMultiply != 1.0f)
            {
                unsigned char multiply_table[256];
                ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);
                stbrp_rect* r = &glyphRect[0];
                for (int glyph_i = 0; glyph_i < glyphCount; glyph_i++, r++)
                    if (r->was_packed)
                        ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, atlas->TexPixelsAlpha8, r->x, r->y, r->w,
            r->h, fontTextureWidth * 1);
            }*/

            // End packing
            stbtt_PackEnd(&spc);
            glyphRect.clear();

            const float font_scale = stbtt_ScaleForPixelHeight(&plexFontInfo, fontSize);
            int unscaled_ascent, unscaled_descent, unscaled_line_gap;
            stbtt_GetFontVMetrics(&plexFontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

            const float ascent = std::floor(unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1));
            // const float descent = std::floor(unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1));

            const float font_off_x = 0.0;
            const float font_off_y = std::round(ascent);

            for (int glyph_i = 0; glyph_i < glyphCount; glyph_i++)
            {
                // Register glyph
                const int codepoint = codepointsFound[glyph_i];
                const stbtt_packedchar &pc = glyphPackedChar[glyph_i];
                stbtt_aligned_quad q;
                float unused_x = 0.0f, unused_y = 0.0f;
                stbtt_GetPackedQuad(glyphPackedChar.data(), fontTextureWidth, fontTextureHeight, glyph_i, &unused_x,
                                    &unused_y, &q, 0);
                AddGlyph(codepoint, q.x0 + font_off_x, q.y0 + font_off_y, q.x1 + font_off_x, q.y1 + font_off_y, q.s0,
                         q.t0, q.s1, q.t1, pc.xadvance);
            }

            const int offset = (int)solidRect.x + (int)solidRect.y * fontTextureWidth;
            glyphTexture[offset] = glyphTexture[offset + 1] = glyphTexture[offset + fontTextureWidth] = glyphTexture[offset + fontTextureWidth + 1] = (std::byte)0xFF;
        }
        else
        {
            std::cerr << "Font invalid" << std::endl;
        }
    }
    else
    {
        std::cerr << "Font not found!" << std::endl;
    }
}

} // namespace UI
} // namespace Engine
