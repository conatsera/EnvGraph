module;

#include <array>

#define NOMINMAX
#include <Windows.h>
export module windows;

import platform;
import pubsub;

namespace EnvGraph {
	namespace Windows {
		export class WindowPlatform {
		public:
			auto Init() -> bool {
				return false;
			}

			auto Deinit() -> void {

			}

			auto GetWindowHandle() const -> HWND {
				return m_window;
			};

			auto GetResolution() const->std::array<int64_t, 2> {
				RECT windowClientRect{ 0,0,0,0 };
				GetClientRect(m_window, &windowClientRect);
				return std::array<int64_t, 2>{
					windowClientRect.right - windowClientRect.left,
						windowClientRect.bottom - windowClientRect.top
				};
			}

			auto SetResolution(uint64_t width, uint64_t height) -> bool { return false; };
			auto CloseWindow() -> void {};
			auto ToggleFullscreen() -> bool { return false; };
			auto Minimize() -> void {};
			auto GetFocus() -> bool { return false;  };
			auto SetFocus() -> bool { return false;  };
//			auto GetMessagePublisher() -> Publisher<Engine::WindowMessage, Engine::WindowMessageFilter> {};

		private:
			HWND m_window;

			static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


		};
	}

	export class AudioIO {

	};
}