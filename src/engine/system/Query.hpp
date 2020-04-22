#pragma once

#include "system/Config.hpp"
#include "Common.hpp"

#include <chrono>

/**
 \brief Perform CPU duration measurement between two time points.
 \ingroup System
 */
class Query {
public:
	

	/** Constructor.
	 */
	Query();

	/** Start measuring the timing. */
	void begin();

	/** End the measurement. */
	void end();

	/** Query the last timing measured. Unit used is nanoseconds.
	 \return the raw metric value */
	uint64_t value();

private:

	std::chrono::time_point<std::chrono::steady_clock> _start; ///< Timing start point.
	std::chrono::time_point<std::chrono::steady_clock> _end; ///< Timing end point.
	bool _running	= false; ///< Is a measurement currently taking place.
};
