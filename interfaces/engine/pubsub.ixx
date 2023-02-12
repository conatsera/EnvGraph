module;

#include <vector>
#include <chrono>
#include <queue>
#include <functional>
#include <execution>
#include <memory>
#include <algorithm>

export module pubsub;

namespace EnvGraph {


export
struct Message {
	const uint64_t timeIndex;
	auto GetTimeIndex() const -> uint64_t { return timeIndex;  }
	Message(uint64_t _timeIndex) : timeIndex(_timeIndex) {
	}
};

export template<typename T>
concept CMessage = requires(T msg) {
	requires(std::is_base_of<Message, T>::value);
	{ msg.GetTimeIndex() } -> std::same_as<uint64_t>;
};

export enum class FilterMode {

};

export template<bool active, typename T, T _value>
struct ValueFilter {
	constexpr const bool IsActive() const { return active; }

	const T value = _value;
	ValueFilter() {}
};


export template<typename T, typename M>
concept CFilter = requires(T filter, std::shared_ptr<M> msg) {
	//{ filter.Get() } -> std::same_as<void>;
	//{ filter & msg } -> std::same_as<bool>;
	{ filter.Match(msg) } -> std::same_as<bool>;
	{ filter.IsActive() } -> std::same_as<bool>;
};



export template<CMessage T, CFilter<T> F>
class Listener {
	using msg_t = std::shared_ptr<T>;
	using Functor = std::function<bool(msg_t&)>;
public:
	Listener() {};
	Listener(F _filter) : filter(_filter) {};
	~Listener() {};

	bool Init() {
		m_functors.reserve(4);
		
		return true;
	}
	void Deinit() {}

	void RegisterCallback(Functor functor) {
		m_functors.push_back(functor);
	}

	void ReceiveMessage(msg_t& msg) {
		//m_queue.push(msg);
		std::for_each(std::execution::par, m_functors.begin(), m_functors.end(), [&msg](Functor functor) { functor(msg); });
	}

private:
	std::vector<Functor> m_functors;
	//std::queue<msg_t> m_queue;
	const F filter;
};

export template<CMessage T, CFilter<T> F>
class Publisher {
	using msg_t = std::shared_ptr<T>;
public:
	Publisher() {}
	~Publisher() {}

	auto CreateListener() -> std::shared_ptr<Listener<T, F>> {
		m_listeners.push_back(std::make_shared<Listener<T, F>>());
		return m_listeners.back();
	}

	auto CreateListener(F filter) -> std::shared_ptr<Listener<T, F>> {
		m_listeners.push_back(std::make_shared<Listener<T, F>>(filter));
		return m_listeners.back();
	}

	auto SendMessage(msg_t &msg) -> bool {
		const bool test = !m_filter.IsActive();
		if (test || m_filter.Match(msg))
		{
			for (auto& listener : m_listeners) {
				listener->ReceiveMessage(msg);
			}
			return true;
		}
		else {
			return false;
		}
	}
private:
	std::vector<std::shared_ptr<Listener<T, F>>> m_listeners;
	F m_filter{};
};


}