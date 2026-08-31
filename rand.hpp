#ifndef RAND_HPP
#define RAND_HPP

#include <random>

#include "int.hpp"
#include "float.hpp"

namespace Rand{
	inline std::mt19937 state;
	inline void seed(u32 s){
		state.seed(s);
	}
	// [0, maximum)
	inline u32 value (u32 maximum) {
		return u64(state()) * maximum >> 32;
	}
	// [minimum, maximum)
	inline u32 value (u32 minimum, u32 maximum) {
		return (u64(state()) * (maximum - minimum) >> 32) + minimum;
	}
	// [0.0, 1.0)
	inline f64 range01() {
		static std::uniform_real_distribution<f64> dist(0.0, 1.0);
		return dist(state);
	}
}

#endif
