#include <cmath>

#include "int.hpp"
#include "float.hpp"
#include "timer.hpp"
#include "rand.hpp"

namespace SA{
	const f64 temp_log_start = 5, temp_log_end = -5;
	f64 temp = 0;
	const u32 attemps_cnt_in_1misec = 1000;
	void core(){
		/*
		u32 r1 = Rand::value(L1),
			r2 = Rand::value(L2),
			r3 = Rand::value(L3);
		f64 new_score = target.simulate(r1, r2, r3);
		f64 diff = new_score - target.score();
		// if(-diff + temp > 0){
		// if(diff < 0 || Rand::range01() < std::exp(-diff / temp)){
			target.execute(r1, r2, r3);
		}
		*/
	}
	void main(u32 time_limit = 1900){
		u32 timer_mili;
		while((timer_mili = Timer::milli()) < time_limit){
			temp = std::exp(temp_log_start + (temp_log_end - temp_log_start) * timer_mili / time_limit);
			for(u32 cnt = 0; cnt < attemps_cnt_in_1misec; ++cnt){
				core();
			}
		}
	}
}
