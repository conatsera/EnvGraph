module;

#include <array>

#define NOMINMAX
#include <Windows.h>

export module dx12;

namespace EnvGraph {
namespace Windows {
export class DirectXPlatform {
public:
	DirectXPlatform() {};
	~DirectXPlatform() {};

	auto Init() -> bool {
		return false;
	}

	auto Deinit() -> void {

	}

	auto EnableComputePass() -> bool {
		return false;
	}
	auto DisableComputePass() -> void {}

	auto EnableRenderPass() -> bool {
		return false;
	}
	auto DisableRenderPass() -> void {}

	auto GetRenderResolution() -> std::array<int64_t, 2> {
		return { 0,0 };
	}
	auto UpdateRenderResolution(auto width, auto height) -> bool {
		return false;
	}
};
}
}