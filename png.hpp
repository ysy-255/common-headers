#ifndef PNG_HPP
#define PNG_HPP

#include <cmath>
#include <array>
// #include <iostream>

#include <zlib.h>

#include "file.hpp"
#include "image.hpp"


#define PNG_MINIMUM_SIZE 57
#define PNG_MINIMUM_CHUNK_SIZE 12

#define PNG_IHDR_SIZE 13
#define PNG_IEND_SIZE 0

/*
特筆すべき事項:
	RGBA8及びRGB8及びindexed-RGB8のみ対応
	IHDR, IDAT, IEND, PLTE以外のチャンクに非対応
	read関数におけるCRC全無視
	エラーハンドリング未実装
*/
// コンパイル時に -lz を指定してください
class PNG{
public:

	enum class Err{
		NONE, // 正常に処理されたはずです
		INCORRECT_SIGNATURE, // シグネチャ(最初の8バイト)が定義されているものと異なります
		UNRECOGNIZABLE, // PNGとして認識できませんでした
		ZLIB_ERROR // ZLIB側のエラーです
	};

	PNG(){};

	PNG(u32 Height, u32 Width)   : H(Height), W(Width), data(H, W) {}
	PNG(const Image_RGB8 & img)  : H(img.H),  W(img.W), data(img),      alpha(false) {}
	PNG(const Image_RGBA8 & img) : H(img.H),  W(img.W), data(img),      alpha(true) {}
	PNG(const PNG & png)         : H(png.H),  W(png.W), data(png.data), alpha(png.alpha) {}

	PNG & operator=(const PNG & other);

	const Image_RGBA8 & ImageData() const{ return data; }
	std::vector<RGBA8> & operator[](const size_t h){ return data[h]; }

	PNG(const std::string & path){ read(path); }

	Err read(const std::string & path);
	// lelel:圧縮レベル(0~9)
	void write(const std::string & path, u8 level = 7);

	u32 H, W;
	bool alpha = false;


protected:

	Image_RGBA8 data;

	bool has_pallet = false;

	std::vector<u8> PNGstream;
	std::vector<u8> filtered_stream;

	std::array<RGBA8, 256> pallet;


	static constexpr std::array<u8, 8> correct_signature = {137, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	static constexpr std::array<u8, 7> colorType2channel = {1, 0, 3, 1, 2, 0, 4};

	static constexpr u32 IHDR_crc = 0xA8'A1'AE'0A;
	static constexpr u32 IDAT_crc = 0x35'AF'06'1E;
	static constexpr u32 IEND_crc = 0xAE'42'60'82;


	bool read_IHDR(const u8* & ptr, z_stream & z){
		if(ptr - PNGstream.data() != 16) return false;
		u8 Bit_depth = 8;
		u8 Color_type = 2; // 2:RGB 6:RGBA
		u8 Compression_method = 0; // 0
		u8 Filter_method = 0; // 0
		u8 Interlace_method = 0; // 0:no 1:has

		W = readBE<u32>(ptr);
		H = readBE<u32>(ptr);
		Bit_depth = *ptr++;
		Color_type = *ptr++;
		Compression_method = *ptr++;
		Filter_method = *ptr++;
		Interlace_method = *ptr++;

		if(Compression_method) return false;
		if(Filter_method) return false;
		if(Interlace_method) return false; // インタレース非対応


		/* 8BIT_RGB, 8BIT_RGBA, 8BIT_PLTE のみ対応 */
		if(Bit_depth != 8) return false;
		if(Color_type != 2 && Color_type != 3 && Color_type != 6) return false;

		alpha = (Color_type == 4 || Color_type == 6);
		has_pallet = (Color_type == 3);
		filtered_stream.resize((1 + W * colorType2channel[Color_type]) * H);
		z.next_out = filtered_stream.data();
		z.avail_out = filtered_stream.size();
		return true;
	}

	bool read_IDAT(const u8* & ptr, const u32 length, z_stream & z){
		if(ptr - PNGstream.data() < 37) return false;
		z.next_in = const_cast<u8*>(ptr);
		z.avail_in = length;
		int ret;
		do{
			ret = inflate(&z, Z_FINISH);
			if(ret != Z_OK && ret != Z_STREAM_END && (ret != Z_BUF_ERROR || z.avail_in > 0)) return false;
		} while(z.avail_in > 0);
		ptr += length;
		return true;
	}

	bool read_PLTE(const u8* & ptr, const u32 length){
		if(ptr - PNGstream.data() < 37) return false;
		if(length > 256 * 3 || length % 3 > 0) return false;
		u16 pallet_size = length / 3;
		for(u16 i = 0; i < pallet_size; ++i){
			pallet[i].R = *ptr++;
			pallet[i].G = *ptr++;
			pallet[i].B = *ptr++;
		}
		return true;
	}

	/*
	   PLTEがある際に必ず使用
	   範囲外の画素インデックスに対しては未定義の値が割り当てられる
	*/
	bool read_indexed_stream(){
		data = Image_RGBA8(H, W);
		u8* ptr = filtered_stream.data();
		for(u32 h = 0; h < H; ++h){
			if(*(ptr++) != 0) return false; // フィルタ方法は(0: none)のみ対応
			for(u32 w = 0; w < W; ++w){
				data[h][w] = pallet[*ptr++];
			}
		}
		return true;
	}


	void write_IHDR(u8* & ptr){
		writeValue<u32>(ptr, PNG_IHDR_SIZE, false);
		*ptr++ = 'I';
		*ptr++ = 'H';
		*ptr++ = 'D';
		*ptr++ = 'R';
		writeValue<u32>(ptr, W, false);
		writeValue<u32>(ptr, H, false);
		*ptr++ = 8;
		*ptr++ = alpha ? 6 : 2;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		u32 crc = crc32(IHDR_crc, ptr - PNG_IHDR_SIZE, PNG_IHDR_SIZE);
		writeValue<u32>(ptr, crc, false);
		return;
	}

	void write_IDAT(u8* & ptr, const std::vector<u8> & deflated_stream){
		size_t deflated_size = deflated_stream.size();
		writeValue<u32>(ptr, deflated_size, false);
		*ptr++ = 'I';
		*ptr++ = 'D';
		*ptr++ = 'A';
		*ptr++ = 'T';
		std::copy(deflated_stream.begin(), deflated_stream.end(), ptr);
		u32 crc = crc32_z(IDAT_crc, ptr, deflated_size);
		ptr += deflated_size;
		writeValue<u32>(ptr, crc, false);
		return;
	}

	void write_IEND(u8* & ptr){
		writeValue<u32>(ptr, PNG_IEND_SIZE, false);
		*ptr++ = 'I';
		*ptr++ = 'E';
		*ptr++ = 'N';
		*ptr++ = 'D';
		writeValue<u32>(ptr, IEND_crc, false);
	}


	// a:left b:above c:upperleft
	inline static u8 paeth_predictor(const u8 a, const u8 c, const u8 b){
		short pb = a, pa = b, pc;
		pa -= c; pb -= c; pc = pa + pb;
		pa = abs(pa); pb = abs(pb); pc = abs(pc);
		if(pa <= pb && pa <= pc) return a;
		if(pb <= pc) return b;
		return c;
	}


	void uf_None(const u32 h, const u8* & ptr){
		for(RGBA8 & p : data[h]){
			p.R += *ptr++;
			p.G += *ptr++;
			p.B += *ptr++;
			if(!alpha) continue;
			p.A += *ptr++;
		}
		return;
	}

	void uf_Sub(const u32 h, const u8* & ptr){
		uf_None(h, ptr);
		if(W <= 1) return;
		auto & line = data[h];
		for(auto itr = line.begin() + 1; itr != line.end(); ++itr){
			itr->R += (itr - 1)->R;
			itr->G += (itr - 1)->G;
			itr->B += (itr - 1)->B;
			if(!alpha) continue;
			itr->A += (itr - 1)->A;
		}
		return;
	}

	void uf_Up(const u32 h, const u8* & ptr){
		if(h > 0) data[h] = data[h - 1];
		uf_None(h, ptr);
		return;
	}

	void uf_Ave(const u32 h, const u8* & ptr){
		if(h > 0) data[h] = data[h - 1];
		auto & line = data[h];
		auto itr = line.begin();
		itr->R >>= 1; itr->G >>= 1; itr->B >>= 1;
		if(alpha) itr->A >>= 1;
		itr->R += *ptr++;
		itr->G += *ptr++;
		itr->B += *ptr++;
		if(alpha) itr->A += *ptr++;
		while(++itr != line.end()){
			itr->R = ((itr->R + (itr - 1)->R) >> 1) + *ptr++;
			itr->G = ((itr->G + (itr - 1)->G) >> 1) + *ptr++;
			itr->B = ((itr->B + (itr - 1)->B) >> 1) + *ptr++;
			if(!alpha) continue;
			itr->A = ((itr->A + (itr - 1)->A) >> 1) + *ptr++;
		}
		return;
	}

	void uf_Paeth(const u32 h, const u8* & ptr){
		if(h == 0){
			uf_Sub(h, ptr);
			return;
		}
		else if(W <= 1){
			uf_Up(h, ptr);
			return;
		}
		auto & line = (*this)[h], &up_line = (*this)[h - 1];
		line[0] = up_line[0];
		std::copy(up_line.begin(), up_line.end() - 1, line.begin() + 1);
		auto itr = line.begin();
		itr->R += *ptr++;
		itr->G += *ptr++;
		itr->B += *ptr++;
		if(alpha) itr->A += *ptr++;
		for(++itr; ++itr != line.end();){
			(itr - 1)->R = paeth_predictor((itr - 2)->R, (itr - 1)->R, itr->R) + *ptr++;
			(itr - 1)->G = paeth_predictor((itr - 2)->G, (itr - 1)->G, itr->G) + *ptr++;
			(itr - 1)->B = paeth_predictor((itr - 2)->B, (itr - 1)->B, itr->B) + *ptr++;
			if(!alpha) continue;
			(itr - 1)->A = paeth_predictor((itr - 2)->A, (itr - 1)->A, itr->A) + *ptr++;
		}
		(itr - 1)->R = paeth_predictor((itr - 2)->R, (itr - 1)->R, up_line.back().R) + *ptr++;
		(itr - 1)->G = paeth_predictor((itr - 2)->G, (itr - 1)->G, up_line.back().G) + *ptr++;
		(itr - 1)->B = paeth_predictor((itr - 2)->B, (itr - 1)->B, up_line.back().B) + *ptr++;
		if(!alpha) return;
		(itr - 1)->A = paeth_predictor((itr - 2)->A, (itr - 1)->A, up_line.back().A) + *ptr++;
		return;
	}

	void (PNG::*uf_funcs[5])(const u32, const u8* &)
	 = {&PNG::uf_None, &PNG::uf_Sub, &PNG::uf_Up, &PNG::uf_Ave, &PNG::uf_Paeth};


	bool f_None(const u32 h, std::vector<u8> & filtered){
		filtered[0] = 0;
		u8 *filtered_ptr = &filtered[1];
		for(const RGBA8 & p : data[h]){
			*filtered_ptr++ = p.R;
			*filtered_ptr++ = p.G;
			*filtered_ptr++ = p.B;
			if(!alpha) continue;
			*filtered_ptr++ = p.A;
		}
		return true;
	}

	bool f_Sub(const u32 h, std::vector<u8> & filtered){
		f_None(h, filtered);
		filtered[0] = 1;
		u8 *filtered_ptr = &filtered[1 + (alpha ? 4 : 3)];
		auto & line = (*this)[h];
		for(auto ptr = line.begin(); ptr != line.end() - 1; ++ptr){
			*filtered_ptr++ -= ptr->R;
			*filtered_ptr++ -= ptr->G;
			*filtered_ptr++ -= ptr->B;
			if(!alpha) continue;
			*filtered_ptr++ -= ptr->A;
		}
		return true;
	}

	bool f_Up(const u32 h, std::vector<u8> & filtered){
		if(h == 0) return false;
		f_None(h, filtered);
		filtered[0] = 2;
		u8 *filtered_ptr = &filtered[1];
		for(const RGBA8 & p : data[h - 1]){
			*filtered_ptr++ -= p.R;
			*filtered_ptr++ -= p.G;
			*filtered_ptr++ -= p.B;
			if(!alpha) continue;
			*filtered_ptr++ -= p.A;
		}
		return true;
	}

	bool f_Ave(const u32 h, std::vector<u8> & filtered){
		if(h == 0) return false;
		size_t index = 0;
		filtered[index ++] = 3;
		u8 sub_R = 0, sub_G = 0, sub_B = 0, sub_A = 0;
		auto & line = data[h], & up_line = data[h - 1];
		for(auto ptr = line.begin(), up_ptr = up_line.begin(); ptr != line.end(); ++ptr, ++up_ptr){
			filtered[index ++] = ptr->R - ((sub_R + up_ptr->R) >> 1); sub_R = ptr->R;
			filtered[index ++] = ptr->G - ((sub_G + up_ptr->G) >> 1); sub_G = ptr->G;
			filtered[index ++] = ptr->B - ((sub_B + up_ptr->B) >> 1); sub_B = ptr->B;
			if(alpha){
				filtered[index ++] = ptr->A - ((sub_A + up_ptr->A) >> 1); sub_A = ptr->A;
			}
		}
		return true;
	}

	bool f_Paeth(const u32 h, std::vector<u8> & filtered){
		if(h == 0) return false;
		size_t index = 0;
		filtered[index ++] = 4;
		auto & line = data[h], &up_line = data[h - 1];
		u8 sub_R = 0, sub_G = 0, sub_B = 0, sub_A = 0,
		        upl_R = 0, upl_G = 0, upl_B = 0, upl_A = 0;
		for(auto ptr = line.begin(), up_ptr = up_line.begin(); ptr != line.end(); ++ptr, ++up_ptr){
			filtered[index ++] = ptr->R - paeth_predictor(sub_R, upl_R, up_ptr->R); sub_R = ptr->R, upl_R = up_ptr->R;
			filtered[index ++] = ptr->G - paeth_predictor(sub_G, upl_G, up_ptr->G); sub_G = ptr->G, upl_G = up_ptr->G;
			filtered[index ++] = ptr->B - paeth_predictor(sub_B, upl_B, up_ptr->B); sub_B = ptr->B, upl_B = up_ptr->B;
			if(!alpha) continue;
			filtered[index ++] = ptr->A - paeth_predictor(sub_A, upl_A, up_ptr->A); sub_A = ptr->A, upl_A = up_ptr->A;
		}
		return true;
	}

	bool (PNG::*f_funcs[5])(const u32, std::vector<u8> &)
	 = {&PNG::f_None, &PNG::f_Sub, &PNG::f_Up, &PNG::f_Ave, &PNG::f_Paeth};


	bool unfilterer(){
		data = Image_RGBA8(H, W);
		u8 filter_type;
		const u8* ptr = filtered_stream.data();
		for(u32 h = 0; h < H; ++h){
			filter_type = *ptr++;
			if(filter_type >= 5) return false;
			(this->*uf_funcs[filter_type])(h, ptr);
		}
		return true;
	}


	u64 abs_sum(const std::vector<u8> & vec){
		u64 res = 0;
		for(const u8 i : vec) res += std::abs(static_cast<int8_t>(i));
		return res;
	}

	void filterer(){
		size_t line_size = 1 + (alpha ? 4 : 3) * W;
		filtered_stream.resize(line_size * H);
		size_t index = 0;
		std::vector<u8> filtered_array[5];
		for(auto & filtered : filtered_array){
			filtered.resize(line_size);
		}
		for(u32 h = 0; h < H; ++h){
			u8 best_filter = 0;
			u64 best_score = UINT64_MAX;
			for(u8 i = 0; i < 5; ++i){
				if((this->*f_funcs[i])(h, filtered_array[i])){
					u64 score = abs_sum(filtered_array[i]);
					if(score < best_score){
						best_score = score;
						best_filter = i;
					}
				}
			}
			std::copy(filtered_array[best_filter].begin(), filtered_array[best_filter].end(), &filtered_stream[index]);
			index += line_size;
		}
		return;
	}


	std::vector<u8> deflate_RLE(std::vector<u8> & src, u8 level = 7){
		size_t dest_size = compressBound(src.size());
		std::vector<u8> res(dest_size);
		z_stream z; z.zalloc = Z_NULL; z.zfree = Z_NULL; z.opaque = Z_NULL;
		if(deflateInit2(
			&z,
			level, // 圧縮レベル
			Z_DEFLATED,
			15, // ウィンドウサイズは最大
			8, // 手元だと 8 が最も良かった
			Z_RLE // ランレングス圧縮
		) != Z_OK) return {};
		z.next_in = src.data();
		z.avail_in = src.size();
		z.next_out = res.data();
		z.avail_out = res.size();
		int ret;
		do{
			ret = deflate(&z, Z_FINISH);
			if(ret != Z_OK && ret != Z_STREAM_END) return {};
		} while(ret != Z_STREAM_END);
		deflateEnd(&z);
		res.resize(z.total_out);
		return res;
	}

};

PNG & PNG::operator=(const PNG & other){
	if(this != &other){
		H = other.H;
		W = other.W;
		data = other.data;
		alpha = other.alpha;
	}
	return *this;
}

PNG::Err PNG::read(const std::string & path){
	PNGstream = readFile(path);
	if(PNGstream.size() < PNG_MINIMUM_SIZE) return Err::UNRECOGNIZABLE;
	const u8* ptr = PNGstream.data();
	if(!std::equal(ptr, ptr + 8, correct_signature.begin())) return Err::INCORRECT_SIGNATURE;
	ptr += 8;

	z_stream z; z.zalloc = Z_NULL; z.zfree = Z_NULL; z.opaque = Z_NULL;
	if(inflateInit(&z) != Z_OK) return Err::ZLIB_ERROR;

	std::string chunk_type(4, '\0');
	do{
		u32 length = readBE<u32>(ptr);
		std::copy(ptr, ptr + 4, chunk_type.data());
		ptr += 4;

		if(chunk_type == "IHDR"){
			if(!read_IHDR(ptr, z)) return Err::UNRECOGNIZABLE;
		}
		else if(chunk_type == "IDAT"){
			if(!read_IDAT(ptr, length, z)) return Err::UNRECOGNIZABLE;
		}
		else if(chunk_type == "IEND"){
			if(ptr - PNGstream.data() < 49) return Err::UNRECOGNIZABLE;
			inflateEnd(&z);
		}
		else if(chunk_type == "PLTE"){
			if(!read_PLTE(ptr, length)) return Err::UNRECOGNIZABLE;
		}
		else{
			if(ptr - PNGstream.data() < 37) return Err::UNRECOGNIZABLE;
			ptr += length;
		}
		ptr += 4; // CRC32
	} while(chunk_type != "IEND" && ptr + PNG_MINIMUM_CHUNK_SIZE <= &*PNGstream.end());

	if(has_pallet){
		if(!read_indexed_stream()) return Err::UNRECOGNIZABLE;
	}
	else{
		if(!unfilterer()) return Err::UNRECOGNIZABLE;
	}

	return Err::NONE;
}

void PNG::write(const std::string & path, u8 level){
	level = std::clamp(static_cast<int>(level), 0, 9);
	filterer();
	auto deflated_stream = deflate_RLE(filtered_stream, level);
	PNGstream.resize(deflated_stream.size() + PNG_MINIMUM_SIZE);
	u8* ptr = PNGstream.data();

	std::copy(correct_signature.begin(), correct_signature.end(), ptr);
	ptr += correct_signature.size();

	write_IHDR(ptr);
	write_IDAT(ptr, deflated_stream);
	write_IEND(ptr);
	writeFile(path, PNGstream);
}

#endif


// 付け足すべき警告等
/*
chunk out-of-range
unknown chunk
 -official
 -critical
 -none critical
zlib error
APNG chunk
unsupported chunk (for this program)
unsupported bitdepth {1, 2, 4, 16}
unsupperted color {grayscale}
unsupperted (interace mode)
no IEND
no IHDR
no IDAT
no PLTE for indexed-color
*/