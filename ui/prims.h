#pragma once
#ifndef GRAPHICS_UI_PRIMS_H
#define GRAPHICS_UI_PRIMS_H

#include <cstdint>
#include <thread>

#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

namespace Engine
{
namespace UI
{

template<unsigned int dims>
struct Vertex
{
    glm::vec<dims, float> pos;
    glm::vec2 uv;
};

template<unsigned int dims> 
struct UIVertex
{
    glm::vec<dims, float> pos;
    glm::vec2 uv;
    glm::vec<4, uint8_t> color;
    vk::Bool32 active;
};

struct LitVertex : Vertex<4>
{
    glm::vec<4, uint8_t> color;
    float reflectivity;
};



template<class vtx>
inline void CreateQuad(vtx *&vertexBuffer,
                       glm::vec2& x, glm::vec2& y,
                       glm::vec2& u, glm::vec2& v,
                       glm::u8vec4 &color)
{
    vertexBuffer[0].pos = glm::vec2{x.x, y.x};
    vertexBuffer[0].color = color;
    vertexBuffer[0].uv = glm::vec2{u.x, v.x};

    vertexBuffer[1].pos = glm::vec2{x.y, y.x};
    vertexBuffer[1].color = color;
    vertexBuffer[1].uv = glm::vec2{u.y, v.x};

    vertexBuffer[2].pos = glm::vec2{x.y, y.y};
    vertexBuffer[2].color = color;
    vertexBuffer[2].uv = glm::vec2{u.y, v.y};

    vertexBuffer[3].pos = glm::vec2{x.y, y.y};
    vertexBuffer[3].color = color;
    vertexBuffer[3].uv = glm::vec2{u.y, v.y};

    vertexBuffer[4].pos = glm::vec2{x.x, y.y};
    vertexBuffer[4].color = color;
    vertexBuffer[4].uv = glm::vec2{u.x, v.y};

    vertexBuffer[5].pos = glm::vec2{x.x, y.x};
    vertexBuffer[5].color = color;
    vertexBuffer[5].uv = glm::vec2{u.x, v.x};
}

template<class vtx, glm::vec2 u, glm::vec2 v>
inline void CreateQuad(vtx *&vertexBuffer, glm::vec2& x, glm::vec2& y, glm::u8vec4 &color)
{
    vertexBuffer[0].pos = glm::vec2{x.x, y.x};
    vertexBuffer[0].color = color;
    vertexBuffer[0].uv = glm::vec2{u.x, v.x};

    vertexBuffer[1].pos = glm::vec2{x.y, y.x};
    vertexBuffer[1].color = color;
    vertexBuffer[1].uv = glm::vec2{u.y, v.x};

    vertexBuffer[2].pos = glm::vec2{x.y, y.y};
    vertexBuffer[2].color = color;
    vertexBuffer[2].uv = glm::vec2{u.y, v.y};

    vertexBuffer[3].pos = glm::vec2{x.y, y.y};
    vertexBuffer[3].color = color;
    vertexBuffer[3].uv = glm::vec2{u.y, v.y};

    vertexBuffer[4].pos = glm::vec2{x.x, y.y};
    vertexBuffer[4].color = color;
    vertexBuffer[4].uv = glm::vec2{u.x, v.y};

    vertexBuffer[5].pos = glm::vec2{x.x, y.x};
    vertexBuffer[5].color = color;
    vertexBuffer[5].uv = glm::vec2{u.x, v.x};
}

template<class vtx>
inline void CreateCube(vtx *vertexBuffer, const size_t &bufferIndex, uint32_t &vertexCount, glm::vec2& x,
                       glm::vec2& y, EngineTexture& tex)
{
    std::array<std::thread, 6> sideThreads;

    int side = 0;
    for (side = 0; side < 6; side++)
    {
        vtx *sideBufferLocation = &vertexBuffer[side * 6];

        sideThreads[side] = std::thread([sideBufferLocation, x, y, tex] {

            CreateQuad<vtx, tex.coords.u,tex.coords.v>(sideBufferLocation, x, y);
            

        });
    }

    for (side = 0; side < 6; side++)
    {
        sideThreads[side].join();
    }
    vertexCount += 6 * 6;
}

} // namespace UI
} // namespace Engine

#endif