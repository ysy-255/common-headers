#include <iostream>
#include <array>

#include "FILE.hpp"

namespace binhex4{
	/* !"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr */
	const std::array<uint8_t, 0x53> table = {
		   0,0x00,0x01,0x02, 0x03,0x04,0x05,0x06, 0x07,0x08,0x09,0x0A, 0x0B,0x0C,   0,   0,
		0x0D,0x0E,0x0F,0x10, 0x11,0x12,0x13,   0, 0x14,0x15,   0,   0,    0,   0,   0,   0,
		0x16,0x17,0x18,0x19, 0x1A,0x1B,0x1C,0x1D, 0x1E,0x1F,0x20,0x21, 0x22,0x23,0x24,   0,
		0x25,0x26,0x27,0x28, 0x29,0x2A,0x2B,   0, 0x2C,0x2D,0x2E,0x2F,    0,   0,   0,   0,
		0x30,0x31,0x32,0x33, 0x34,0x35,0x36,   0, 0x37,0x38,0x39,0x3A, 0x3B,0x3C,   0,   0,
		0x3D,0x3E,0x3F
	};
	const std::string correct_comment = "(This file must be converted with BinHex 4.0)";
	std::vector<uint8_t> read_simple(const std::string & path){
		const auto raw = readFile(path);
		auto l = raw.begin(), r = raw.end();
		l = std::find(l, r, '(');
		if(r - l < correct_comment.size() + 3) return {};
		if(!std::equal(correct_comment.begin(), correct_comment.end(), l)) return {};
		l += correct_comment.size();
		l = std::find(l, r, ':');
		if(r - l < 2) return {};
		l ++;
		while(l != r) if(*--r == ':') break;
		std::vector<uint8_t> stream; stream.reserve(r - l);

		bool mode90 = false;
		auto f = [&](){
			if(mode90){
				uint8_t times = stream.back();
				stream.pop_back(); // times
				if(times != 0){
					times --;
					stream.pop_back(); // 0x90
					uint8_t ch = stream.back();
					stream.resize(stream.size() + times, ch);
				}
				mode90 = false;
			}
			else if(stream.back() == 0x90){
				mode90 = true;
			}
		};
		for(uint8_t cnt = 0; l != r; l++){
			uint8_t ch = *l;
			ch -= 0x20;
			if(ch >= 0x53) continue;
			ch = table[ch];
			switch(cnt++ & 0b11){
				case 0:{
					stream.push_back(ch << 2);
					break;
				}
				case 1:{
					stream.back() |= ch >> 4;
					f();
					stream.push_back(ch << 4);
					break;
				}
				case 2:{
					stream.back() |= ch >> 2;
					f();
					stream.push_back(ch << 6);
					break;
				}
				case 3:{
					stream.back() |= ch >> 0;
					f();
					break;
				}
			}
		}

		size_t idx = stream.front() + 12;
		const uint8_t* i = &stream[idx];
		size_t sz = readData<uint32_t>(i, false);
		size_t res_l = std::min(idx + 10, stream.size());
		size_t res_r = std::min(idx + 10 + sz, stream.size());
		return std::vector<uint8_t>(stream.begin() + res_l, stream.begin() + res_r);
	}
}
