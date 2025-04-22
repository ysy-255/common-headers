#include <iostream>

#include "FILE.hpp"

namespace binhex4{
	constexpr uint8_t table[] = {
		0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
		0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
		   0,0x00,0x01,0x02, 0x03,0x04,0x05,0x06, 0x07,0x08,0x09,0x0A, 0x0B,0x0C,   0,   0,
		0x0D,0x0E,0x0F,0x10, 0x11,0x12,0x13,   0, 0x14,0x15,   0,   0,    0,   0,   0,   0,
		0x16,0x17,0x18,0x19, 0x1A,0x1B,0x1C,0x1D, 0x1E,0x1F,0x20,0x21, 0x22,0x23,0x24,   0,
		0x25,0x26,0x27,0x28, 0x29,0x2A,0x2B,   0, 0x2C,0x2D,0x2E,0x2F,    0,   0,   0,   0,
		0x30,0x31,0x32,0x33, 0x34,0x35,0x36,   0, 0x37,0x38,0x39,0x3A, 0x3B,0x3C,   0,   0,
		0x3D,0x3E,0x3F
	};
	std::string correct_comment = "(This file must be converted with BinHex 4.0)";
	std::vector<uint8_t> read_simple(const std::string & path){
		std::ifstream ifs = std::ifstream(path);
		std::string comment;
		std::getline(ifs, comment);
		while(!comment.empty()){
			if(comment.back() == '\r'){
				comment.pop_back();
			}
			else break;
		}
		if(comment != correct_comment) return {};
		char start;
		ifs >> start;
		if(start != ':') return {};
		std::string raw, line;
		while(ifs >> line){
			raw += line;
		}
		if(raw.size() < 2) return {};
		if(raw.back() == ':') raw.pop_back();
		std::vector<uint8_t> stream; stream.reserve(raw.size());
		bool mode90 = false;
		auto f = [&](){
			if(mode90){
				uint8_t times = stream.back();
				stream.pop_back();
				if(times != 0){
					times --;
					stream.pop_back();
					uint8_t ch = stream.back();
					stream.resize(stream.size() + times, ch);
				}
				mode90 = false;
			}
			else if(stream.back() == 0x90){
				mode90 = true;
			}
		};
		for(auto itr = raw.begin(), end = raw.begin() + (raw.size() >> 2 << 2); itr != end; itr += 4){
			stream.emplace_back(table[*(itr + 0)] << 2 | table[*(itr + 1)] >> 4);
			f();
			stream.emplace_back(table[*(itr + 1)] << 4 | table[*(itr + 2)] >> 2);
			f();
			stream.emplace_back(table[*(itr + 2)] << 6 | table[*(itr + 3)] >> 0);
			f();
		}
		// ↑あまりは無視しました
		
		size_t idx = stream.front() + 12;
		const uint8_t* i = &stream[idx];
		size_t sz = readData<uint32_t>(i, false);
		idx += 10;
		std::vector<uint8_t> res;
		res.reserve(sz);
		for(size_t i = 0; i < sz; ++i){
			res.emplace_back(stream[idx + i]);
		}
		return res;
	}
}
