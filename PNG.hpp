#ifndef PNG_HPP
#define PNG_HPP

#include <cmath>
#include <array>
// #include <iostream>

#include <zlib.h>

#include "FILE.hpp"
#include "IMAGE.hpp"


#define PNG_MINIMUM_SIZE 57
#define PNG_MINIMUM_CHUNK_SIZE 12

#define PNG_IHDR_SIZE 13
#define PNG_IEND_SIZE 0

// コンパイル時に -lz を指定してください
class PNG{
	public:

	PNG(){};

	PNG(const uint32_t W, const uint32_t H) :
		Width(W), Height(H), ImageData(W, H) {}

	PNG(const IMAGE_RGB<uint8_t> & img) :
		Width(img.width), Height(img.height), ImageData(img), has_alpha(false) {}

	PNG(const IMAGE_RGBA<uint8_t> & img) :
		Width(img.width), Height(img.height), ImageData(img), has_alpha(true) {}

	PNG(const PNG & png) :
		Width(png.Width), Height(png.Height), ImageData(png.ImageData), has_alpha(png.has_alpha) {}

	PNG(const std::string & path){
		read(path);
	}

	PNG & operator=(const PNG & other){
		if(this != &other){
			Width = other.Width;
			Height = other.Height;
			ImageData = other.ImageData;
			has_alpha = other.has_alpha;
		}
		return *this;
	}



	uint32_t Width;
	uint32_t Height;

	IMAGE_RGBA<uint8_t> ImageData;

	bool has_alpha = false;


	inline std::vector<RGBA<uint8_t>> & operator[](size_t h){
		return ImageData[h];
	}



	bool read(const std::string & path){
		PNGstream = readFile(path);
		if(PNGstream.size() < PNG_MINIMUM_SIZE) return false;
		const uint8_t* ptr = PNGstream.data();
		if(!std::equal(ptr, ptr + 8, correct_signature.begin())) return false;
		ptr += 8;

		z_stream z; z.zalloc = Z_NULL; z.zfree = Z_NULL; z.opaque = Z_NULL;
		if(inflateInit(&z) != Z_OK) return false;

		std::string chunk_type(4, '\0');
		do{
			uint32_t length = readData<uint32_t>(ptr, false);
			std::copy(ptr, ptr + 4, chunk_type.data());
			ptr += 4;

			if(chunk_type == "IHDR"){
				if(!read_IHDR(ptr, z)) return false;
			}
			else if(chunk_type == "IDAT"){
				if(!read_IDAT(ptr, length, z)) return false;
			}
			else if(chunk_type == "IEND"){
				if(ptr - PNGstream.data() < 49) return false;
				inflateEnd(&z);
			}
			else if(chunk_type == "PLTE"){
				if(!read_PLTE(ptr, length)) return false;
			}
			else{
				if(ptr - PNGstream.data() < 37) return false;
				ptr += length;
			}
			ptr += 4; // CRC32
		} while(chunk_type != "IEND" && ptr + PNG_MINIMUM_CHUNK_SIZE <= &*PNGstream.end());

		if(has_pallet){
			if(!read_indexed_stream()) return false;
		}
		else{
			if(!unfilterer()) return false;
		}

		return true;
	}

	// lelel:圧縮レベル(0~9)
	bool write(const std::string & path, uint8_t level = 7){
		level = std::clamp(static_cast<int>(level), 0, 9);
		filterer();
		auto deflated_stream = deflate_RLE(filtered_stream, level);
		PNGstream.resize(deflated_stream.size() + PNG_MINIMUM_SIZE);
		uint8_t* ptr = PNGstream.data();

		std::copy(correct_signature.begin(), correct_signature.end(), ptr);
		ptr += correct_signature.size();

		write_IHDR(ptr);
		write_IDAT(ptr, deflated_stream);
		write_IEND(ptr);

		writeFile(path, PNGstream);

		return true;
	}



	protected:

	bool has_pallet = false;

	std::vector<uint8_t> PNGstream;
	std::vector<uint8_t> filtered_stream;

	std::array<RGBA<uint8_t>, 256> pallet;


	static constexpr std::array<uint8_t, 8> correct_signature = {137, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	static constexpr std::array<uint8_t, 7> colorType2channel = {1, 0, 3, 1, 2, 0, 4};

	static constexpr uint32_t IHDR_crc = 0xA8'A1'AE'0A;
	static constexpr uint32_t IDAT_crc = 0x35'AF'06'1E;
	static constexpr uint32_t IEND_crc = 0xAE'42'60'82;


	bool read_IHDR(const uint8_t* & ptr, z_stream & z){
		if(ptr - PNGstream.data() != 16) return false;
		uint8_t Bit_depth = 8;
		uint8_t Color_type = 2; // 2:RGB 6:RGBA
		uint8_t Compression_method = 0; // 0
		uint8_t Filter_method = 0; // 0
		uint8_t Interlace_method = 0; // 0:no 1:has

		Width = readData<uint32_t>(ptr, false);
		Height = readData<uint32_t>(ptr, false);
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

		has_alpha = (Color_type == 4 || Color_type == 6);
		has_pallet = (Color_type == 3);
		filtered_stream.resize((1 + Width * colorType2channel[Color_type]) * Height);
		z.next_out = filtered_stream.data();
		z.avail_out = filtered_stream.size();
		return true;
	}

	bool read_IDAT(const uint8_t* & ptr, const uint32_t length, z_stream & z){
		if(ptr - PNGstream.data() < 37) return false;
		z.next_in = const_cast<uint8_t*>(ptr);
		z.avail_in = length;
		int ret;
		do{
			ret = inflate(&z, Z_FINISH);
			if(ret != Z_OK && ret != Z_STREAM_END && (ret != Z_BUF_ERROR || z.avail_in > 0)) return false;
		} while(z.avail_in > 0);
		ptr += length;
		return true;
	}

	bool read_PLTE(const uint8_t* & ptr, const uint32_t length){
		if(ptr - PNGstream.data() < 37) return false;
		if(length > 256 * 3 || length % 3 > 0) return false;
		uint16_t pallet_size = length / 3;
		for(uint16_t i = 0; i < pallet_size; ++i){
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
		ImageData = IMAGE_RGBA<uint8_t>(Width, Height);
		uint8_t* ptr = filtered_stream.data();
		for(uint32_t h = 0; h < Height; ++h){
			if(*(ptr++) != 0) return false; // フィルタ方法は(0: none)のみ対応
			for(uint32_t w = 0; w < Width; ++w){
				(*this)[h][w] = pallet[*ptr++];
			}
		}
		return true;
	}


	void write_IHDR(uint8_t* & ptr){
		writeData<uint32_t>(ptr, PNG_IHDR_SIZE, false);
		*ptr++ = 'I';
		*ptr++ = 'H';
		*ptr++ = 'D';
		*ptr++ = 'R';
		writeData<uint32_t>(ptr, Width, false);
		writeData<uint32_t>(ptr, Height, false);
		*ptr++ = 8;
		*ptr++ = has_alpha ? 6 : 2;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		uint32_t crc = crc32(IHDR_crc, ptr - PNG_IHDR_SIZE, PNG_IHDR_SIZE);
		writeData<uint32_t>(ptr, crc, false);
		return;
	}

	void write_IDAT(uint8_t* & ptr, const std::vector<uint8_t> & deflated_stream){
		size_t deflated_size = deflated_stream.size();
		writeData<uint32_t>(ptr, deflated_size, false);
		*ptr++ = 'I';
		*ptr++ = 'D';
		*ptr++ = 'A';
		*ptr++ = 'T';
		std::copy(deflated_stream.begin(), deflated_stream.end(), ptr);
		uint32_t crc = crc32_z(IDAT_crc, ptr, deflated_size);
		ptr += deflated_size;
		writeData<uint32_t>(ptr, crc, false);
		return;
	}

	void write_IEND(uint8_t* & ptr){
		writeData<uint32_t>(ptr, PNG_IEND_SIZE, false);
		*ptr++ = 'I';
		*ptr++ = 'E';
		*ptr++ = 'N';
		*ptr++ = 'D';
		writeData<uint32_t>(ptr, IEND_crc, false);
	}


	// a:left b:above c:upperleft
	inline static uint8_t paeth_predictor(const uint8_t a, const uint8_t c, const uint8_t b){
		short pb = a, pa = b, pc;
		pa -= c; pb -= c; pc = pa + pb;
		pa = abs(pa); pb = abs(pb); pc = abs(pc);
		if(pa <= pb && pa <= pc) return a;
		if(pb <= pc) return b;
		return c;
	}


	void uf_None(const uint32_t h, const uint8_t* & ptr){
		for(RGBA<uint8_t> & p : (*this)[h]){
			p.R += *ptr++;
			p.G += *ptr++;
			p.B += *ptr++;
			if(!has_alpha) continue;
			p.A += *ptr++;
		}
		return;
	}

	void uf_Sub(const uint32_t h, const uint8_t* & ptr){
		uf_None(h, ptr);
		if(Width <= 1) return;
		auto & line = (*this)[h];
		for(auto itr = line.begin() + 1; itr != line.end(); ++itr){
			itr->R += (itr - 1)->R;
			itr->G += (itr - 1)->G;
			itr->B += (itr - 1)->B;
			if(!has_alpha) continue;
			itr->A += (itr - 1)->A;
		}
		return;
	}

	void uf_Up(const uint32_t h, const uint8_t* & ptr){
		if(h > 0) (*this)[h] = (*this)[h - 1];
		uf_None(h, ptr);
		return;
	}

	void uf_Ave(const uint32_t h, const uint8_t* & ptr){
		if(h > 0) (*this)[h] = (*this)[h - 1];
		auto & line = (*this)[h];
		auto itr = line.begin();
		itr->R >>= 1; itr->G >>= 1; itr->B >>= 1;
		if(has_alpha) itr->A >>= 1;
		itr->R += *ptr++;
		itr->G += *ptr++;
		itr->B += *ptr++;
		if(has_alpha) itr->A += *ptr++;
		while(++itr != line.end()){
			itr->R = ((itr->R + (itr - 1)->R) >> 1) + *ptr++;
			itr->G = ((itr->G + (itr - 1)->G) >> 1) + *ptr++;
			itr->B = ((itr->B + (itr - 1)->B) >> 1) + *ptr++;
			if(!has_alpha) continue;
			itr->A = ((itr->A + (itr - 1)->A) >> 1) + *ptr++;
		}
		return;
	}

	void uf_Paeth(const uint32_t h, const uint8_t* & ptr){
		if(h == 0){
			uf_Sub(h, ptr);
			return;
		}
		else if(Width <= 1){
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
		if(has_alpha) itr->A += *ptr++;
		for(++itr; ++itr != line.end();){
			(itr - 1)->R = paeth_predictor((itr - 2)->R, (itr - 1)->R, itr->R) + *ptr++;
			(itr - 1)->G = paeth_predictor((itr - 2)->G, (itr - 1)->G, itr->G) + *ptr++;
			(itr - 1)->B = paeth_predictor((itr - 2)->B, (itr - 1)->B, itr->B) + *ptr++;
			if(!has_alpha) continue;
			(itr - 1)->A = paeth_predictor((itr - 2)->A, (itr - 1)->A, itr->A) + *ptr++;
		}
		(itr - 1)->R = paeth_predictor((itr - 2)->R, (itr - 1)->R, up_line.back().R) + *ptr++;
		(itr - 1)->G = paeth_predictor((itr - 2)->G, (itr - 1)->G, up_line.back().G) + *ptr++;
		(itr - 1)->B = paeth_predictor((itr - 2)->B, (itr - 1)->B, up_line.back().B) + *ptr++;
		if(!has_alpha) return;
		(itr - 1)->A = paeth_predictor((itr - 2)->A, (itr - 1)->A, up_line.back().A) + *ptr++;
		return;
	}

	void (PNG::*uf_funcs[5])(const uint32_t, const uint8_t* &)
	 = {&PNG::uf_None, &PNG::uf_Sub, &PNG::uf_Up, &PNG::uf_Ave, &PNG::uf_Paeth};


	bool f_None(const uint32_t h, std::vector<uint8_t> & filtered){
		filtered[0] = 0;
		uint8_t *filtered_ptr = &filtered[1];
		for(const RGBA<uint8_t> & p : (*this)[h]){
			*filtered_ptr++ = p.R;
			*filtered_ptr++ = p.G;
			*filtered_ptr++ = p.B;
			if(!has_alpha) continue;
			*filtered_ptr++ = p.A;
		}
		return true;
	}

	bool f_Sub(const uint32_t h, std::vector<uint8_t> & filtered){
		f_None(h, filtered);
		filtered[0] = 1;
		uint8_t *filtered_ptr = &filtered[1 + (has_alpha ? 4 : 3)];
		auto & line = (*this)[h];
		for(auto ptr = line.begin(); ptr != line.end() - 1; ++ptr){
			*filtered_ptr++ -= ptr->R;
			*filtered_ptr++ -= ptr->G;
			*filtered_ptr++ -= ptr->B;
			if(!has_alpha) continue;
			*filtered_ptr++ -= ptr->A;
		}
		return true;
	}

	bool f_Up(const uint32_t h, std::vector<uint8_t> & filtered){
		if(h == 0) return false;
		f_None(h, filtered);
		filtered[0] = 2;
		uint8_t *filtered_ptr = &filtered[1];
		for(const RGBA<uint8_t> & p : (*this)[h - 1]){
			*filtered_ptr++ -= p.R;
			*filtered_ptr++ -= p.G;
			*filtered_ptr++ -= p.B;
			if(!has_alpha) continue;
			*filtered_ptr++ -= p.A;
		}
		return true;
	}

	bool f_Ave(const uint32_t h, std::vector<uint8_t> & filtered){
		if(h == 0) return false;
		size_t index = 0;
		filtered[index ++] = 3;
		uint8_t sub_R = 0, sub_G = 0, sub_B = 0, sub_A = 0;
		auto & line = (*this)[h], & up_line = (*this)[h - 1];
		for(auto ptr = line.begin(), up_ptr = up_line.begin(); ptr != line.end(); ++ptr, ++up_ptr){
			filtered[index ++] = ptr->R - ((sub_R + up_ptr->R) >> 1); sub_R = ptr->R;
			filtered[index ++] = ptr->G - ((sub_G + up_ptr->G) >> 1); sub_G = ptr->G;
			filtered[index ++] = ptr->B - ((sub_B + up_ptr->B) >> 1); sub_B = ptr->B;
			if(has_alpha){
				filtered[index ++] = ptr->A - ((sub_A + up_ptr->A) >> 1); sub_A = ptr->A;
			}
		}
		return true;
	}

	bool f_Paeth(const uint32_t h, std::vector<uint8_t> & filtered){
		if(h == 0) return false;
		size_t index = 0;
		filtered[index ++] = 4;
		auto & line = (*this)[h], &up_line = (*this)[h - 1];
		uint8_t sub_R = 0, sub_G = 0, sub_B = 0, sub_A = 0,
		        upl_R = 0, upl_G = 0, upl_B = 0, upl_A = 0;
		for(auto ptr = line.begin(), up_ptr = up_line.begin(); ptr != line.end(); ++ptr, ++up_ptr){
			filtered[index ++] = ptr->R - paeth_predictor(sub_R, upl_R, up_ptr->R); sub_R = ptr->R, upl_R = up_ptr->R;
			filtered[index ++] = ptr->G - paeth_predictor(sub_G, upl_G, up_ptr->G); sub_G = ptr->G, upl_G = up_ptr->G;
			filtered[index ++] = ptr->B - paeth_predictor(sub_B, upl_B, up_ptr->B); sub_B = ptr->B, upl_B = up_ptr->B;
			if(!has_alpha) continue;
			filtered[index ++] = ptr->A - paeth_predictor(sub_A, upl_A, up_ptr->A); sub_A = ptr->A, upl_A = up_ptr->A;
		}
		return true;
	}

	bool (PNG::*f_funcs[5])(const uint32_t, std::vector<uint8_t> &)
	 = {&PNG::f_None, &PNG::f_Sub, &PNG::f_Up, &PNG::f_Ave, &PNG::f_Paeth};


	bool unfilterer(){
		ImageData = IMAGE_RGBA<uint8_t>(Width, Height);
		uint8_t filter_type;
		const uint8_t* ptr = filtered_stream.data();
		for(uint32_t h = 0; h < Height; ++h){
			filter_type = *ptr++;
			if(filter_type >= 5) return false;
			(this->*uf_funcs[filter_type])(h, ptr);
		}
		return true;
	}


	uint64_t abs_sum(const std::vector<uint8_t> & vec){
		uint64_t res = 0;
		for(const uint8_t i : vec) res += std::abs(static_cast<int8_t>(i));
		return res;
	}

	void filterer(){
		size_t line_size = 1 + (has_alpha ? 4 : 3) * Width;
		filtered_stream.resize(line_size * Height);
		size_t index = 0;
		std::vector<uint8_t> filtered_array[5];
		for(auto & filtered : filtered_array){
			filtered.resize(line_size);
		}
		for(uint32_t h = 0; h < Height; ++h){
			uint8_t best_filter = 0;
			uint64_t best_score = UINT64_MAX;
			for(uint8_t i = 0; i < 5; ++i){
				if((this->*f_funcs[i])(h, filtered_array[i])){
					uint64_t score = abs_sum(filtered_array[i]);
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


	std::vector<uint8_t> deflate_RLE(std::vector<uint8_t> & src, uint8_t level = 7){
		size_t dest_size = compressBound(src.size());
		std::vector<uint8_t> res(dest_size);
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

#endif
