#include "system/Query.hpp"

Query::Query() {

}

void Query::begin() {
	if(_running) {
		Log::Warning() << "A query is already running. Ignoring the restart." << std::endl;
		return;
	}
	_start	 = std::chrono::high_resolution_clock::now();
	_running = true;
}

void Query::end() {
	if(!_running) {
		Log::Warning() << "No query running currently. Ignoring the stop." << std::endl;
		return;
	}
	_end	 = std::chrono::high_resolution_clock::now();
	_running = false;
}

uint64_t Query::value() {
	if(_running) {
		Log::Warning() << "A query is currently running, stopping it first." << std::endl;
		end();
	}
	const long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count();
	return uint64_t(duration);
}
