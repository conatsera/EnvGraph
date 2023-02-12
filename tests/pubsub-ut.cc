import pubsub;

#include <array>
#include <string>
#include <memory>
#include <execution>
#include <semaphore>
#include <random>

#include <gtest/gtest.h>

namespace {

using namespace EnvGraph;

struct MockMessage : Message {
	const bool  boolVal;
	const int   intVal;
	const float floatVal;
	std::array<int64_t, 4> arrayVal;
	std::string stringVal = std::string("");

	MockMessage() : boolVal(false), intVal(0), floatVal(0.f), arrayVal(std::array<int64_t, 4>()), Message(0) {

	}

	MockMessage(
		bool _boolVal,
		int _intVal,
		float _floatVal,
		const std::array<int64_t, 4> & _arrayVal,
		const std::string & _stringVal) :
		boolVal(_boolVal),
		intVal(_intVal),
		floatVal(_floatVal),
		arrayVal(std::array<int64_t, 4>()),
		Message(0) {
		std::copy(std::execution::par, _arrayVal.begin(), _arrayVal.end(), arrayVal.begin());
		stringVal.resize(_stringVal.size());
		std::copy(_stringVal.begin(), _stringVal.end(), stringVal.begin());
	}

	//MockMessage(const MockMessage& msg) : MockMessage(msg.boolVal, msg.intVal, msg.floatVal, msg.arrayVal, msg.stringVal) {}
	//MockMessage(MockMessage& msg) : MockMessage(msg.boolVal, msg.intVal, msg.floatVal, msg.arrayVal, msg.stringVal) {}
	/*MockMessage& operator=(const MockMessage msg) {
		
		*this = MockMessage(msg.boolVal, msg.intVal, msg.floatVal, msg.arrayVal, msg.stringVal);
		return *this;
	}*/
};

template<bool active, typename T, T value>
using VF = ValueFilter<active, T, value>;
template<typename T, T value>
using AVF = ValueFilter<true, T, value>;
template<typename T, T value>
using IVF = ValueFilter<false, T, value>;

template<typename boolVal_t, typename intVal_t, typename floatVal_t>
struct MockFilter {
	boolVal_t  boolValFilter{};
	intVal_t   intValFilter{};
	floatVal_t floatValFilter{};
	//IVF<decltype(MockMessage::arrayVal), arrayVal>	  arrayValFilter;
	constexpr const bool IsActive() const {
		return (boolValFilter.IsActive() || intValFilter.IsActive() || floatValFilter.IsActive());
	}
	constexpr auto Match(std::shared_ptr<MockMessage>& msg) const -> bool {
        const uint8_t checkMask = ((uint8_t)boolValFilter.IsActive() | ((uint8_t)intValFilter.IsActive() << 1) |
                                ((uint8_t)floatValFilter.IsActive() << 2));
		if (checkMask == 0) return true;

		uint8_t resSum = 0;
		if (boolValFilter.IsActive())
		{
            const auto res = (uint8_t)(msg->boolVal == boolValFilter.value);
			resSum |= res;
		}
        resSum |= static_cast<uint8_t>(intValFilter.IsActive() && (msg->intVal == intValFilter.value)) << 1;
        resSum |= static_cast<uint8_t>(floatValFilter.IsActive() && (msg->floatVal == floatValFilter.value)) << 2;
		
		const auto checkResult = (resSum | checkMask);
		
		return (checkResult == resSum);
	}
};

using MockPublisher = Publisher<MockMessage, MockFilter<IVF<bool, false>, IVF<int, 0>, IVF<float, 0.f>>>;
using MockFilteredPublisher = Publisher<MockMessage, MockFilter<AVF<bool, true>, IVF<int, 0>, IVF<float, 0.f>>>;

class PubSubTests : public ::testing::Test {
public:
	void SetUp() {
		m_testMockPublisher = std::make_unique<MockPublisher>();
		m_testMockFilteredPublisher = std::make_unique<MockFilteredPublisher>();
		m_rand = std::minstd_rand(m_rd());
	}

	void TearDown() {
		
	}

protected:
	std::unique_ptr<MockPublisher> m_testMockPublisher;
	std::unique_ptr<MockFilteredPublisher> m_testMockFilteredPublisher;

	const MockMessage GenerateMessage() {
		const bool _boolVal = m_uniformBoolVals(m_rand) == 1; // (i & 0x1);
		const int _intVal = m_uniformIntVals(m_rand);// (i & (INT_MAX >> 1)) << 1;
		const float _floatVal = m_uniformFloatVals(m_rand);
		const auto _arrayVal = std::array<int64_t, 4>();
		return MockMessage(_boolVal, _intVal, _floatVal, _arrayVal, "mock message");
	}
private:
	std::random_device m_rd;
	std::minstd_rand m_rand;

	std::uniform_int<int>    m_uniformBoolVals  = std::uniform_int<int>(0, 1);
	std::uniform_int<int>    m_uniformIntVals   = std::uniform_int<int>(0,256);
	std::uniform_real<float> m_uniformFloatVals = std::uniform_real<float>(0.0, 1.0);
};

TEST(PubSub, CreateDestroyPublishers) {
	auto testMockPublisher = std::make_unique<MockPublisher>();
	auto testMockFilteredPublisher = std::make_unique<MockFilteredPublisher>();

}

TEST_F(PubSubTests, ReceiveAllMessages) {
	std::vector<std::shared_ptr<MockMessage>> allMessagesReceived;
	allMessagesReceived.reserve(1024);
	{
		auto testMockPublisher = std::make_unique<MockPublisher>();
		auto allMessageListener = testMockPublisher->CreateListener();
		ASSERT_EQ(allMessageListener->Init(), true);

		std::binary_semaphore allMessageSemaphore(1);

		allMessageListener->RegisterCallback([&allMessagesReceived, &allMessageSemaphore](std::shared_ptr<MockMessage>& msg) -> bool {
			allMessagesReceived.push_back(msg);
			allMessageSemaphore.release();
			return true;
		});

		std::vector<std::shared_ptr<MockMessage>> randomMessages;
		randomMessages.resize(1024);
		std::for_each(randomMessages.begin(), randomMessages.end(), [&](std::shared_ptr<MockMessage>& msg) { msg = std::make_shared<MockMessage>(GenerateMessage()); testMockPublisher->SendMessage(msg); });

		while (allMessageSemaphore.try_acquire_for(std::chrono::milliseconds(2))) {
		}
		ASSERT_EQ(allMessagesReceived.size(), 1024);
	}
}

TEST(PubSub, ReceiveFilteredPublisherMessages) {
	auto testMockFilteredPublisher = std::make_unique<MockFilteredPublisher>();
	auto allMessageListener = testMockFilteredPublisher->CreateListener();
	ASSERT_EQ(allMessageListener->Init(), true);

	std::binary_semaphore allMessageSemaphore(1);
	std::vector<std::shared_ptr<MockMessage>> allMessagesReceived;
	allMessagesReceived.reserve(4);

	allMessageListener->RegisterCallback([&allMessagesReceived, &allMessageSemaphore](std::shared_ptr<MockMessage>& msg) -> bool {
		allMessagesReceived.push_back(msg);
		allMessageSemaphore.release();
		return true;
	});
	const auto messageArray = std::array<int64_t, 4>();
	auto passingMessage = std::make_shared<MockMessage>(true,  1, 1.f, messageArray, "passing test");
	auto failingMessage = std::make_shared<MockMessage>(false, 1, 1.f, messageArray, "failing test");
	testMockFilteredPublisher->SendMessage(passingMessage);

	ASSERT_EQ(allMessageSemaphore.try_acquire_for(std::chrono::seconds(1)), true);
	ASSERT_EQ(allMessagesReceived.size(), 1);

	testMockFilteredPublisher->SendMessage(failingMessage);
	
	ASSERT_EQ(allMessageSemaphore.try_acquire_for(std::chrono::milliseconds(1)), false);
	ASSERT_EQ(allMessagesReceived.size(), 1);

	testMockFilteredPublisher->SendMessage(passingMessage);

	ASSERT_EQ(allMessageSemaphore.try_acquire_for(std::chrono::seconds(1)), true);
	ASSERT_EQ(allMessagesReceived.size(), 2);
}

TEST(PubSub, ReceieveFilteredListenerMessages) {
	auto testMockPublisher = std::make_unique<MockPublisher>();
	//auto testFilteredListener = testMockPublisher->CreateListener<MockFilter<AVF<bool, true>, IVF<int, 0>, IVF<float, 0.f>>>();
}

//auto boolValFilteredListener = testMockPublisher->CreateListener(MockFilter<AVF<bool, true>, IVF<int, 0>, IVF<float, 0.f>>());

}