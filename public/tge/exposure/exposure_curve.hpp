#pragma once

#include <tge/assert.hpp>
#include <tge/math/functions.hpp>

#include <array>
#include <cmath>
#include <cstdint>

namespace Tge::Exposure
{
struct SExposureCurveKey final
{
	float ev100{ 0.0f };
	float stops{ 0.0f };
};

inline constexpr uint32_t MaxExposureCurveKeys{ 16u };

class CExposureCurve final
{
public:

	CExposureCurve() = default;
	~CExposureCurve() = default;

	bool AddKey(float const ev100, float const stops)
	{
		bool added{ false };

		if (std::isfinite(ev100) && std::isfinite(stops))
		{
			uint32_t slot{ 0u };

			while ((slot < m_numKeys) && (m_keys[slot].ev100 < (ev100 - DuplicateEpsilon)))
			{
				++slot;
			}

			bool const replaces = (slot < m_numKeys) && (Math::Abs(m_keys[slot].ev100 - ev100) <= DuplicateEpsilon);

			if (replaces)
			{
				m_keys[slot] = SExposureCurveKey{ ev100, stops };
				added        = true;
			}
			else if (m_numKeys < MaxExposureCurveKeys)
			{
				for (uint32_t shift = m_numKeys; shift > slot; --shift)
				{
					m_keys[shift] = m_keys[shift - 1u];
				}

				m_keys[slot] = SExposureCurveKey{ ev100, stops };
				++m_numKeys;
				added = true;
			}
		}

		return added;
	}

	bool RemoveKey(uint32_t const index)
	{
		bool const removed = (index < m_numKeys);

		if (removed)
		{
			for (uint32_t shift = index; (shift + 1u) < m_numKeys; ++shift)
			{
				m_keys[shift] = m_keys[shift + 1u];
			}

			--m_numKeys;
		}

		return removed;
	}

	void Clear() { m_numKeys = 0u; }

	uint32_t GetNumKeys() const { return m_numKeys; }

	SExposureCurveKey GetKey(uint32_t const index) const
	{
		TGE_ASSERT(index < m_numKeys, "CExposureCurve::GetKey out of range");

		return m_keys[index];
	}

	float Evaluate(float const ev100) const
	{
		float result{ 0.0f };

		if (m_numKeys > 0u)
		{
			result = m_keys[m_numKeys - 1u].stops;

			if (ev100 <= m_keys[0].ev100)
			{
				result = m_keys[0].stops;
			}
			else
			{
				for (uint32_t segment = 0u; (segment + 1u) < m_numKeys; ++segment)
				{
					SExposureCurveKey const& low  = m_keys[segment];
					SExposureCurveKey const& high = m_keys[segment + 1u];

					if ((ev100 > low.ev100) && (ev100 <= high.ev100))
					{
						float const span = high.ev100 - low.ev100;
						float const t    = (span > 0.0f) ? ((ev100 - low.ev100) / span) : 0.0f;

						result = Math::Lerp(low.stops, high.stops, t);
					}
				}
			}
		}

		return result;
	}

private:

	static constexpr float DuplicateEpsilon{ 1e-4f };

	std::array<SExposureCurveKey, MaxExposureCurveKeys> m_keys{};

	uint32_t m_numKeys{ 0u };
};

// The keys are stored inline, so MaxExposureCurveKeys is ABI rather than a tuning constant.
static_assert(sizeof(SExposureCurveKey) ==   8 && alignof(SExposureCurveKey) == 4);
static_assert(sizeof(CExposureCurve)    == 132 && alignof(CExposureCurve)    == 4);
} // namespace Tge::Exposure
