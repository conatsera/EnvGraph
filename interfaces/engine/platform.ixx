module;

#include <concepts>
#include <array>
#include <vector>

export module platform;

import pubsub;

namespace EnvGraph {
namespace Engine {



template<typename T>
concept CPlatform = requires(T platform) {
	{ platform.Init() } -> std::same_as<bool>;
	{ platform.Deinit() } -> std::same_as<void>;
};

export struct WindowMessage : Message {

};

export class WindowMessageFilter {
	//void Get() {};
	//auto operator&(WindowMessage) const -> bool { return false; };
	auto Match(WindowMessage msg) -> bool { return false; };
};

export template<typename T>
concept CWindowPlatform = requires(T window) {
	requires CPlatform<T>;
	{ window.GetResolution() } -> std::convertible_to<std::array<std::int64_t, 2>>;
	{ window.SetResolution(1920, 1080) } -> std::same_as<bool>;
	{ window.CloseWindow() } -> std::same_as<void>;
	{ window.ToggleFullscreen() } -> std::same_as<bool>;
	{ window.Minimize() } -> std::same_as<void>;
	{ window.GetFocus() } -> std::same_as<bool>;
	{ window.SetFocus() } -> std::same_as<bool>;
//	{ window.GetMessagePublisher() } -> 
//		std::same_as<Publisher<WindowMessage, WindowMessageFilter>>;
};

export template<typename T>
concept CGPUPlatform = requires(T gpuPlatform) {
	requires CPlatform<T>;
	{ gpuPlatform.EnableComputePass() } -> std::same_as<bool>;
	{ gpuPlatform.DisableComputePass() } -> std::same_as<void>;
	{ gpuPlatform.EnableRenderPass() } -> std::same_as<bool>;
	{ gpuPlatform.DisableRenderPass() } -> std::same_as<void>;

	{ gpuPlatform.GetRenderResolution() } -> std::same_as<std::array<std::int64_t, 2>>;
	{ gpuPlatform.UpdateRenderResolution(1920, 1080) } -> std::same_as<bool>;
};

export template<typename T>
concept CAudioDevice = requires(T audioDevice) {
	requires CPlatform<T>;
	{ audioDevice.GetSampleBuffer() } -> std::convertible_to<std::vector<uint16_t>>;

};

} // namespace Engine
} // namespace EnvGraph