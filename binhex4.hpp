#ifndef BINHEX4_HPP
#define BINHEX4_HPP

#include <iostream>

#include "file.hpp"

namespace BinHex4{

	std::vector<u8> read(const std::string & path);

	std::string last_FileName;
	u8 last_Version;
	std::string last_Type;
	std::string last_Creator;
	u16 last_Flags;
	std::vector<u8> last_Resource;

	enum class Err;
	enum Err last_error;

	enum class Err{
		NONE, // 正常に処理されたはずです
		INCORRECT_COMMENT, // 最初のコメント行が間違っています
		UNRECOGNIZABLE, // BinHex4.0として認識できませんでした
		OUT_OF_TABLE, // BinHex4.0で定義されているテーブル外の英数記号が用いられています
		FAULTY_DATA, // 解凍後のデータに不備がありました
		CRC_ERROR // データの欠損があるかもしれません
	};

	namespace detail{
		#define XX 0xFF
		/* !"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr */
		const u8 table[] = {
			  XX,0x00,0x01,0x02, 0x03,0x04,0x05,0x06, 0x07,0x08,0x09,0x0A, 0x0B,0x0C,  XX,  XX,
			0x0D,0x0E,0x0F,0x10, 0x11,0x12,0x13,  XX, 0x14,0x15,  XX,  XX,   XX,  XX,  XX,  XX,
			0x16,0x17,0x18,0x19, 0x1A,0x1B,0x1C,0x1D, 0x1E,0x1F,0x20,0x21, 0x22,0x23,0x24,  XX,
			0x25,0x26,0x27,0x28, 0x29,0x2A,0x2B,  XX, 0x2C,0x2D,0x2E,0x2F,   XX,  XX,  XX,  XX,
			0x30,0x31,0x32,0x33, 0x34,0x35,0x36,  XX, 0x37,0x38,0x39,0x3A, 0x3B,0x3C,  XX,  XX,
			0x3D,0x3E,0x3F,  XX,   XX,  XX,  XX,  XX,   XX,  XX,  XX,  XX,   XX,  XX,  XX,  XX
		};
		#undef XX

		const std::string correct_comment = "(This file must be converted with BinHex 4.0)";
		const u8 comment_size = correct_comment.size();

		enum Err trimLR(
			std::vector<u8>::const_iterator & l,
			std::vector<u8>::const_iterator & r
		){
			l = std::find(l, r, '(');
			if(l + comment_size > r) return Err::UNRECOGNIZABLE;
			if(!std::equal(l, l + comment_size, correct_comment.data())) return Err::INCORRECT_COMMENT;
			l += comment_size;
			l = std::find(l, r, ':');
			l ++;
			if(l > r) return Err::UNRECOGNIZABLE;
			while(l != r) if(*--r == ':') break;
			if(l > r) return Err::UNRECOGNIZABLE;
			return Err::NONE;
		}

		enum Err decode(
			std::vector<u8>::const_iterator & l,
			std::vector<u8>::const_iterator & r,
			std::vector<u8> & dst
		){
			bool mode90 = false;
			u8 cnt = 0;
			while(l != r){
				u8 ch = *l++;
				ch -= 0x20;
				if(ch >= 0x60) continue;
				ch = table[ch];
				if(ch == 0xFF) return Err::OUT_OF_TABLE;
				if(cnt > 0){
					dst.back() |= ch >> (6 - cnt);
					if(mode90){
						u8 times = dst.back();
						dst.pop_back(); // times
						if(times != 0){
							times --;
							dst.pop_back(); // 0x90
							u8 ch = dst.back();
							dst.resize(dst.size() + times, ch);
						}
						mode90 = false;
					}
					else{
						mode90 = dst.back() == 0x90;
					}
				}
				cnt += 2;
				if(cnt < 8){
					dst.push_back(ch << cnt);
				}
				cnt &= 0b110;
			}
			return Err::NONE;
		}

		enum Err extract(const std::vector<u8> & src, std::vector<u8> & dst){
			auto itr = src.begin(), end = src.end();
			if(itr + 26 >= end) return Err::FAULTY_DATA;
			u8 FileName_len = *itr++;
			if(itr + FileName_len + 25 >= end) return Err::FAULTY_DATA;
			last_FileName = readString(itr, FileName_len);
			last_Version = *itr++;
			last_Type = readString(itr, 4);
			last_Creator = readString(itr, 4);
			last_Flags = readBE<u16>(itr);
			u32 Data_len = readBE<u32>(itr);
			u32 Rsrc_len = readBE<u32>(itr);
			u16 crc1 = readBE<u16>(itr);
			if(itr + Data_len > end) return Err::FAULTY_DATA;
			dst = readBytes(itr, Data_len);
			if(itr + 2 > end) return Err::FAULTY_DATA;
			u16 crc2 = readBE<u16>(itr);
			if(itr + Rsrc_len > end) return Err::FAULTY_DATA;
			last_Resource = readBytes(itr, Rsrc_len);
			if(itr + 2 > end) return Err::FAULTY_DATA;
			u16 crc3 = readBE<u16>(itr);
			return Err::NONE;
		}
	}

	std::vector<u8> read(const std::string & path){
		using namespace detail;

		const auto raw = readFile(path);
		auto l = raw.begin(), r = raw.end();
		if((last_error = trimLR(l, r)) != Err::NONE) return {};

		std::vector<u8> stream;
		stream.reserve(r - l);
		if((last_error = decode(l, r, stream)) != Err::NONE) return {};

		std::vector<u8> res;
		if((last_error = extract(stream, res)) != Err::NONE) return {};

		return res;
	}
}

/*
	[1] Length of FileName (1 ~ 63);
	[Length of FileName]  FileName;
	[1] Version;
	[4] Type;
	[4] Creator;
	[2] Flags;
	[4] Length of Data;
	[4] Length of Resource;
	[2] CRC;
	[Length of Data] Data;
	[2] CRC;
	[Length of Resource] Resource;
	[2] CRC;
*/

#endif
