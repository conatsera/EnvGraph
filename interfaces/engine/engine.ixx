export module engine;

import platform;
import windows;
import dx12;

namespace EnvGraph {
namespace Engine {

template<CWindowPlatform TWindowPlatform, CGPUPlatform TGPUPlatform>
class EngineImpl {
public:
	EngineImpl() {};
	~EngineImpl() {};

	bool Init();

	void Start();
	void Stop();

	void Pause();

private:
	TGPUPlatform m_gpuPlatforms;


};

#if _WIN32
using DirectXEngine = EngineImpl<Windows::WindowPlatform, Windows::DirectXPlatform>;
//using VulkanEngine =  EngineImpl<Windows::WindowPlatform, VulkanPlatform>;
export using Engine = DirectXEngine;
#elif _UNIX
export using Engine = EngineImpl<>;
#endif

} // namespace Engine
} // namespace EnvGraph
