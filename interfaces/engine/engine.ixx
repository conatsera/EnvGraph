module;

export module engine;

import std.compat;

import log;
import platform;
import windows;
import dx12;

namespace EnvGraph {
namespace Engine {

enum class EngineMode {
	Recovery,
	Test,
	Main
};

struct Recovery {

	void ChangeMode(EngineMode mode);
};

template<EngineMode kEngineMode, CWindowPlatform TWindowPlatform, CGPUPlatform TGPUPlatform>
class EngineImpl {
public:
	EngineImpl() {};
	~EngineImpl() {};

	bool Init() {
        //auto current_path = std::filesystem::current_path().append("logs");
        m_logger.Init(true, "./logs");
        return false;
	}
	void Start();
	void Stop();

	void Pause();

private:
	Recovery m_recovery;
	TGPUPlatform m_gpuPlatform;
    TWindowPlatform m_windowPlatform;

	Logger m_logger;
};

#if _WIN32
template<EngineMode mode>
using DirectXEngine = EngineImpl<mode, Windows::WindowPlatform, Windows::DirectXGPUPlatform>;
//using VulkanEngine =  EngineImpl<Windows::WindowPlatform, VulkanPlatform>;
export using MainEngine = DirectXEngine<EngineMode::Main>;
export using RecoveryEngine = DirectXEngine<EngineMode::Recovery>;
export using TestEngine = DirectXEngine<EngineMode::Test>;
#elif _UNIX
export using Engine = EngineImpl<>;
#endif

} // namespace Engine
} // namespace EnvGraph
