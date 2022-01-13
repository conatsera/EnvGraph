#ifndef ENVGRAPH_COMMON_H
#define ENVGRAPH_COMMON_H

#include <iostream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_double.hpp>

namespace EnvGraph
{

typedef glm::uvec2 Extent;
typedef glm::vec2 Location2D;
typedef glm::vec3 Location3D;
typedef glm::qua<float> QuaternionF;
typedef glm::qua<double> Quaternion;

std::ostream& operator<<(std::ostream& os, const Extent& obj);
std::ostream& operator<<(std::ostream& os, const Location2D& obj);
std::ostream& operator<<(std::ostream& os, const Location3D& obj);
std::ostream& operator<<(std::ostream& os, const QuaternionF& obj);
std::ostream& operator<<(std::ostream& os, const Quaternion& obj);

enum class GpuApiSetting
{
	VULKAN,
	DIRECTX12
};

#ifndef GPU_API_SETTING
#define GPU_API_SETTING 0
#endif

constexpr const char kEngineName[] = "EnvGraph Engine";
constexpr const GpuApiSetting kGpuApiSetting = (GpuApiSetting)GPU_API_SETTING;

}

#endif