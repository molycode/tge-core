#include <gtest/gtest.h>
#include <tge/threading/mpsc_queue.hpp>

#include <cstdint>
#include <thread>
#include <vector>

using Tge::Threading::CMpscQueue;

//////////////////////////////////////////////////////////////////////////
TEST(MpscQueue, ConcurrentProducersLoseNothing)
{
	constexpr int NumProducers{ 4 };
	constexpr int NumPerProducer{ 1000 };

	CMpscQueue<int> queue;
	std::vector<std::thread> producers{};

	for (int producer{ 0 }; producer < NumProducers; ++producer)
	{
		producers.emplace_back([&queue]()
		{
			for (int index{ 1 }; index <= NumPerProducer; ++index)
			{
				queue.Enqueue(index);
			}
		});
	}

	for (std::thread& producer : producers)
	{
		producer.join();
	}

	int numReceived{ 0 };
	int64_t sum{ 0 };
	int value{ 0 };

	while (queue.Dequeue(value))
	{
		++numReceived;
		sum += value;
	}

	EXPECT_EQ(numReceived, NumProducers * NumPerProducer);
	EXPECT_EQ(sum, int64_t{ NumProducers } * NumPerProducer * (NumPerProducer + 1) / 2);
}
