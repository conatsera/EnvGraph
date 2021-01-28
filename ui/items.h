#pragma once
#ifndef GRAPHICS_UI_BUTTON_H
#define GRAPHICS_UI_BUTTON_H

#include <cstddef>
#include <string>
#include <optional>

#include <glm/gtx/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../engine_defs.h"

#include "prims.h"

namespace Engine
{
namespace UI
{

constexpr const size_t kLabelVertexCount = 6;
constexpr const size_t kInitialLabelVertexBufferSize = 64 * sizeof(UIVertex<2>) * 6;

typedef uint32_t LabelID;

constexpr const LabelID kInvalidLabelID = UINT32_MAX;

typedef uint32_t ButtonID;

constexpr const ButtonID kInvalidButtonID = UINT32_MAX;

struct Object
{
    Object(glm::vec3 position, glm::vec3 scale, glm::quat orientation, EngineTexture_t texture) : position(position), scale(scale), orientation(orientation), texture(texture) {}
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat orientation;

    EngineTexture_t texture;

    unsigned int startVertex;
    unsigned int endVertex;
};

enum TextAlign
{
    LEFT = 0,
    CENTER,
    RIGHT
};

struct Label : Object
{
    Label(glm::vec3 position, glm::vec3 scale, glm::quat orientation, EngineTexture_t texture, std::string text, TextAlign alignment = TextAlign::LEFT) : Object(position, scale, orientation, texture), text(text), alignment(alignment) {}

    std::string text;
    TextAlign alignment;
};

struct Button : Object
{
    Button(glm::vec3 position, glm::vec3 scale, glm::quat orientation, EngineTexture_t texture, Label label) : Object(position, scale, orientation, texture), label(label) {}
    std::optional<Label> label;
};

} // namespace UI
} // namespace Engine

#endif