#pragma once

#include <cstdint>
#include <utility>

namespace Tge::Threading
{
enum class EJobPriority : uint8_t
{
	High,
	Normal,
	Low
};

class IJob
{
public:

	virtual ~IJob() = default;
	virtual void Execute() = 0;
};

template<typename Func>
class CLambdaJob final : public IJob
{
public:

	explicit CLambdaJob(Func&& func)
		: m_func(std::forward<Func>(func))
	{
	}

	void Execute() override
	{
		m_func();
	}

private:

	Func m_func;
};
} // namespace Tge::Threading
