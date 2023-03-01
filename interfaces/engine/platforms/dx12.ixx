module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3DCompiler.h>
#include <DirectXMath.h>

#include <wrl.h>
#include <shellapi.h>

//#include <array>

export module dx12;

import std.compat;

import asset;
import platform;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace EnvGraph {
	namespace Windows {



class DirectXPipeline
{
};

export class DirectXGPUPlatform {
public:
	DirectXGPUPlatform() {};
	~DirectXGPUPlatform() {};

	auto Init() -> bool {
		ReadyHardware();

		LoadPipelines();

		return false;
	}

	Engine::RunFunctor Run() {
		return []() -> bool {
			return false;
		};
	}

	void Destroy() {

	}

	Engine::PlatformState GetState() {
		return m_platformState;
	}

	bool Render() {
		return false;
	}

	bool Compute() {
		return false;
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
private:

	void ReadyHardware() {

	}

	bool LoadPipelines() {
		UINT dxgiFactorFlags = 0;

#if defined(_DEBUG)

#endif
		return false;
	}


private:
	Engine::PlatformState m_platformState = Engine::PlatformState::Offline;
};
}
}