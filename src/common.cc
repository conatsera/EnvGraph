#include "common.h"

#include <iostream>

namespace EnvGraph
{
    
std::ostream& operator<<(std::ostream& os, const Extent& obj)
{
	os << "{ Width: " << obj.x << ", Height: " << obj.y << " }";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Location2D& obj)
{
	os << "{ X: " << obj.x << ", Y: " << obj.y << " }";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Location3D& obj)
{
	os << "{ X: " << obj.x << ", Y: " << obj.y << ", Z: " << obj.z << " }";
	return os;
}

std::ostream& operator<<(std::ostream& os, const QuaternionF& obj)
{
	os << "{ W: " << obj.w << ", X: " << obj.x << ", Y: " << obj.y << ", Z: " << obj.z << " }";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Quaternion& obj)
{
	os << "{ W: " << obj.w << ", X: " << obj.x << ", Y: " << obj.y << ", Z: " << obj.z << " }";
	return os;
}
} // namespace EnvGraph