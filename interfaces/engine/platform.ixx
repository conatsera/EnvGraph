module;

/*
#include <concepts>
#include <array>
#include <optional>
#include <functional>
*/
export module platform;

import std.compat;
import pubsub;

namespace EnvGraph {
namespace Engine {

export enum PlatformState {
	Online,
	Sleep,
	Offline,
	Stalled,
	Warning,
	Error,
	Emergency
};

export using RunFunctor = std::function<bool(void)>;

template<typename T>
concept CLifetime = requires(T platform) {
	{ platform.Init() } -> std::same_as<bool>;
	{ platform.Run() } -> std::same_as<RunFunctor>;
	{ platform.Destroy() } -> std::same_as<void>;
	{ platform.GetState() } -> std::same_as<PlatformState>;
};

template<typename T>
concept CRenderer = requires(T renderer) {
	{ renderer.render() } -> std::same_as<bool>;
};

template<typename T>
concept CApp = requires(T app) {
	requires CLifetime<T>;
	
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
	requires CLifetime<T>;
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
	requires CLifetime<T>;
	{ gpuPlatform.EnableComputePass() } -> std::same_as<bool>;
	{ gpuPlatform.DisableComputePass() } -> std::same_as<void>;
	{ gpuPlatform.EnableRenderPass() } -> std::same_as<bool>;
	{ gpuPlatform.DisableRenderPass() } -> std::same_as<void>;

	{ gpuPlatform.GetRenderResolution() } -> std::same_as<std::array<std::int64_t, 2>>;
	{ gpuPlatform.UpdateRenderResolution(1920, 1080) } -> std::same_as<bool>;

	{ gpuPlatform.Render() } -> std::convertible_to<bool>;
	{ gpuPlatform.Compute() } -> std::convertible_to<bool>;
};

} // namespace Engine
} // namespace EnvGraph