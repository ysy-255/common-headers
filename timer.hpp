#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

#include "int.hpp"

namespace Timer{
	using std::chrono::steady_clock;
	inline steady_clock::time_point start_time = steady_clock::now();
	inline void start(){
		start_time = steady_clock::now();
	}
	inline u64 milli(){
		return std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock::now() - start_time).count();
	}
	inline u64 micro(){
		return std::chrono::duration_cast<std::chrono::microseconds>(steady_clock::now() - start_time).count();
	}
	inline u64 nano(){
		return std::chrono::duration_cast<std::chrono::nanoseconds>(steady_clock::now() - start_time).count();
	}
}

#endif
