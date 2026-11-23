#include <gtest/gtest.h>
#include <tge/threading/mpsc_queue.hpp>

#include <memory>

using Tge::Threading::CMpscQueue;

//////////////////////////////////////////////////////////////////////////
TEST(MpscQueue, DequeuesInTheOrderEnqueued)
{
	CMpscQueue<int> queue;
	int value{ 0 };

	queue.Enqueue(1);
	queue.Enqueue(2);

	ASSERT_TRUE(queue.Dequeue(value));
	EXPECT_EQ(value, 1);
	ASSERT_TRUE(queue.Dequeue(value));
	EXPECT_EQ(value, 2);
}

//////////////////////////////////////////////////////////////////////////
TEST(MpscQueue, EmptyQueueYieldsNothing)
{
	CMpscQueue<int> queue;
	int value{ 0 };

	EXPECT_FALSE(queue.Dequeue(value));
}

//////////////////////////////////////////////////////////////////////////
TEST(MpscQueue, CarriesMoveOnlyValues)
{
	CMpscQueue<std::unique_ptr<int>> queue;
	std::unique_ptr<int> value{};

	queue.Enqueue(std::make_unique<int>(7));

	ASSERT_TRUE(queue.Dequeue(value));
	ASSERT_NE(value, nullptr);
	EXPECT_EQ(*value, 7);
}

//////////////////////////////////////////////////////////////////////////
TEST(MpscQueue, KeepsNoCopyOfADequeuedValue)
{
	CMpscQueue<std::shared_ptr<int>> queue;
	std::shared_ptr<int> const original{ std::make_shared<int>(7) };
	std::shared_ptr<int> value{};

	queue.Enqueue(original);
	ASSERT_TRUE(queue.Dequeue(value));
	value.reset();

	EXPECT_EQ(original.use_count(), 1);
}

//////////////////////////////////////////////////////////////////////////
TEST(MpscQueue, DestructionReleasesQueuedValues)
{
	std::shared_ptr<int> const original{ std::make_shared<int>(7) };

	{
		CMpscQueue<std::shared_ptr<int>> queue;

		queue.Enqueue(original);
		queue.Enqueue(original);
	}

	EXPECT_EQ(original.use_count(), 1);
}
