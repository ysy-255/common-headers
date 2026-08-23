#ifndef PNG_HPP
#define PNG_HPP

#include <cmath>
#include <array>
#include <iostream>
using std::clog; using std::cerr;

#include "file.hpp"
#include "image.hpp"
#include "enumflags.hpp"
#include "simplezlib.hpp"

using std::abs;
using std::array;


template<u8 bit_depth>
concept is_png_subbyte = (bit_depth == 1 || bit_depth == 2 || bit_depth == 4);

enum class PNG3_Err{
	NONE, // 正常に処理されたはずです
	INCORRECT_SIGNATURE, // シグネチャ(最初の8バイト)が定義されているものと異なります
	CRC_ERROR, // CRC32が一致しません
	UNKNOWN_CRITICAL_CHUNK, // 未知のクリティカルチャンクが存在します
	MISSING_CRITICAL_CHUNK, // 必須のクリティカルチャンクが存在しません
	FORBIDDEN_CHUNK, // 条件下で存在してはいけないチャンクが存在します
	INVALID_CHUNK_ORDER, // チャンクの順序が不正です
	UNRECOGNIZABLE, // PNGとして認識できませんでした
	ZLIB_ERROR // ZLIB側のエラーです
};

enum class PNG3_Chunk{
	IHDR, PLTE, IDAT, IEND,
	acTL, cHRM, cICP, gAMA, iCCP, mDCV, cLLI, sBIT, sRGB,
	bKGD, hIST, tRNS, eXIf, fcTL, pHYs, sPLT, fdAT,
	tIME, iTXt, tEXt, zTXt,
	COUNT
};


/**
 *特筆すべき事項:
 * `IHDR`, `IDAT`, `IEND`, `PLTE`以外のチャンクに非対応
 * 途中でエラーを吐いた際にinflateEnd()をせずに終了してしまう
 */
// コンパイル時に `-lz` を指定してください
template<pixel_type Pixel> class PNG3;

using PNG = PNG3<RGBA8>;

template<pixel_type Pixel>
class PNG3{
	using T = typename pixel_traits<Pixel>::channel_type;
public:
	using Err = PNG3_Err;

	PNG3() = default;
	PNG3 (u32 Height, u32 Width) : data(Height, Width) {}
	PNG3 (const Image<Pixel>& img) : data(img) {}

	template<pixel_type OtherPixel> requires (!std::same_as<OtherPixel, Pixel>)
	PNG3 (const PNG3<OtherPixel>& other) : data(other.ImageData()) {}

	template<pixel_type OtherPixel> requires (!std::same_as<OtherPixel, Pixel>)
	PNG3 (const Image<OtherPixel>& img) : data(img) {}

	template<pixel_type OtherPixel> requires (!std::same_as<OtherPixel, Pixel>)
	PNG3& operator= (const PNG3<OtherPixel>& other) {
		data = other.ImageData();
		return *this;
	}

	const Image<Pixel>& ImageData() const{ return data; }
	span<Pixel> operator[] (const size_t h) { return data[h]; }
	span<const Pixel> operator[] (const size_t h) const{ return data[h]; }

	u32 height() const { return data.height(); }
	u32 width() const { return data.width(); }

	PNG3 (const string& path) { read(path); }

	Err read (const string& path) {
		vector<u8> PNGstream = readFile(path);
		if (PNGstream.size() < MINIMUM_FILE_SIZE) return Err::UNRECOGNIZABLE;
		vector<u8>::const_iterator itr = PNGstream.begin();
		if (!std::equal(itr, itr + SIGNATURE_SIZE, correct_signature.begin())) return Err::INCORRECT_SIGNATURE;
		itr += SIGNATURE_SIZE;

		SimpleInflate inflater;
		if (!inflater) return Err::ZLIB_ERROR;

		ChunkFlags chunk_flags;
		bool last_IDAT = false;
		u32 W = 0, H = 0;
		u8 bit_depth = 0, color_type = 0, interlace_method = 0;
		vector<u8> filtered_stream;
		array<RGBA8, 256> palette_base{};
		span<const RGBA8> palette;
		do {
			u32 length = readBE<u32>(itr);
			if (length > FOUR_BYTE_LIMIT) return Err::UNRECOGNIZABLE;
			if (4 + length + 4 > PNGstream.end() - itr) return Err::UNRECOGNIZABLE;
			if (crc32_z(0, &*itr, 4 + length) != peekValue<u32, std::endian::big>(itr + 4 + length)) return Err::CRC_ERROR;
			string chunk_string = readString(itr, 4);

			if (chunk_string == "IHDR") {
				if (!check_chunk_order_and_set(chunk_flags, Chunk::IHDR)) return Err::INVALID_CHUNK_ORDER;
				if (!read_IHDR(itr, length, W, H, bit_depth, color_type, interlace_method, inflater, filtered_stream)) return Err::UNRECOGNIZABLE;
			} else if (chunk_string == "PLTE") {
				if (!check_chunk_order_and_set(chunk_flags, Chunk::PLTE)) return Err::INVALID_CHUNK_ORDER;
				if ((color_type & 0b00000010) == 0) return Err::FORBIDDEN_CHUNK;
				if (!read_PLTE(itr, length, palette_base, palette, bit_depth)) return Err::UNRECOGNIZABLE;
			} else if (chunk_string == "IDAT") {
				if (chunk_flags.is_set(Chunk::IDAT) && !last_IDAT) return Err::INVALID_CHUNK_ORDER;
				if (!check_chunk_order_and_set(chunk_flags, Chunk::IDAT)) return Err::INVALID_CHUNK_ORDER;
				if (color_type == 3 && !chunk_flags.is_set(Chunk::PLTE)) return Err::MISSING_CRITICAL_CHUNK;
				if (!read_IDAT(itr, length, inflater)) return Err::UNRECOGNIZABLE;
			} else if (chunk_string == "IEND") {
				if (!check_chunk_order_and_set(chunk_flags, Chunk::IEND)) return Err::INVALID_CHUNK_ORDER;
				if (!read_IEND(itr, length)) return Err::UNRECOGNIZABLE;
				break;
			} else if (chunk_string[0] & 0b00100000) {
				if (chunk_flags.is_clear(Chunk::IHDR)) return Err::INVALID_CHUNK_ORDER;
				itr += length;
			} else {
				return Err::UNKNOWN_CRITICAL_CHUNK;
			}
			itr += 4; // CRC32
			last_IDAT = chunk_string == "IDAT";
		} while (itr < PNGstream.end());

		if (chunk_flags.is_clear(Chunk::IEND)) return Err::MISSING_CRITICAL_CHUNK;

		if (!inflater.all_completed()) return Err::UNRECOGNIZABLE;

		data = Image<Pixel>(H, W);

		if (color_type == 3) {
			if (!read_indexed_data(filtered_stream.begin(), H, W, bit_depth, interlace_method, palette)) return Err::UNRECOGNIZABLE;
		} else {
			if (!read_direct_data(filtered_stream.begin(), H, W, bit_depth, color_type, interlace_method)) return Err::UNRECOGNIZABLE;
		}

		return Err::NONE;
	}

	// lelel:圧縮レベル(0~9)
	void write (const string& path, u8 level = 7) {
		level = std::clamp(level, u8(0), u8(9));
		constexpr u8 bit_depth = limits<T>::digits;
		constexpr u8 color_type = (pixel_traits<Pixel>::rgb ? 2 : 0) | (pixel_traits<Pixel>::alpha ? 4 : 0);
		const u32 W = data.width();
		const u32 H = data.height();
		const u8 offset = bit_depth < 8 ? 1 : colortype2channel[color_type] * bit_depth / 8;
		const size_t scanline_size = calc_scanline_size(W, bit_depth, color_type);
		vector<u8> filtered_stream(calc_filtered_size(H, W, bit_depth, color_type));
		write_data(filtered_stream.begin());
		filterer(filtered_stream.end(), offset, scanline_size, H);
		vector<u8> PNGstream;
		const size_t head_offset = SIGNATURE_SIZE + (4 + 4 + IHDR_SIZE + 4) + (4 + 4);
		const size_t tail_offset = (4) + (4 + 4 + IEND_SIZE + 4);
		deflate_RLE(filtered_stream, PNGstream, head_offset, tail_offset, level);
		const size_t IDAT_size = PNGstream.size() - head_offset - tail_offset;
		vector<u8>::iterator itr = PNGstream.begin();
		copy(correct_signature.begin(), correct_signature.end(), itr);
		itr += correct_signature.size();
		write_IHDR(itr, W, H, bit_depth, color_type);
		write_IDAT(itr, IDAT_size);
		write_IEND(itr);
		writeFile(path, PNGstream);
	}


protected:

	Image<Pixel> data;

	static constexpr u32 SIGNATURE_SIZE = 8;
	static constexpr u32 MINIMUM_FILE_SIZE = 57;
	static constexpr u32 MINIMUM_CHUNK_SIZE = 12;
	static constexpr u32 IHDR_SIZE = 13;
	static constexpr u32 IEND_SIZE = 0;
	static constexpr u32 FILTER_TYPE_SIZE = 1;
	static constexpr u32 FOUR_BYTE_LIMIT = (1U << 31) - 1;

	static constexpr array<u8, SIGNATURE_SIZE> correct_signature = {0b10001001, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

	static constexpr array<u8, 8> colortype2channel = {1, 0, 3, 1, 2, 0, 4, 0};

	static constexpr array<u8, 9> adam7_offsets = {0, 0, 4, 0, 2, 0 ,1, 0, 0};
	static constexpr array<u8, 9> adam7_deltas = {8, 8, 8, 4, 4, 2, 2, 1, 1};


	static constexpr u32 IHDR_crc = 0xA8'A1'AE'0A;
	static constexpr u32 IDAT_crc = 0x35'AF'06'1E;
	static constexpr u32 IEND_crc = 0xAE'42'60'82;

	static size_t calc_pixel_count (const u32 extent, const u32 start, const u32 delta) {
		return (extent - start + delta - 1) / delta;
	}

	static size_t calc_scanline_size (const u32 pixel_count_x, const u8 bit_depth, const u8 color_type) {
		return (pixel_count_x * colortype2channel[color_type] * bit_depth + 7) / 8;
	}

	static size_t calc_filtered_size (const u32 pixel_count_y, const u32 pixel_count_x, const u8 bit_depth, const u8 color_type) {
		if (pixel_count_y == 0 || pixel_count_x == 0) return 0;
		return pixel_count_y * (FILTER_TYPE_SIZE + calc_scanline_size(pixel_count_x, bit_depth, color_type));
	}

	using Chunk = PNG3_Chunk;

	using ChunkFlags = EnumFlags<Chunk, Chunk::COUNT>;

	static constexpr ChunkFlags multiple_disallowed =
	 ~ ChunkFlags({
		Chunk::IDAT, Chunk::fcTL, Chunk::sPLT, Chunk::fdAT, Chunk::iTXt, Chunk::tEXt, Chunk::zTXt
	});

	static constexpr ChunkFlags before_PLTE =
	ChunkFlags({
		Chunk::IHDR, Chunk::cHRM, Chunk::cICP, Chunk::gAMA, Chunk::iCCP, Chunk::mDCV, Chunk::cLLI, Chunk::sBIT, Chunk::sRGB
	});

	static constexpr ChunkFlags after_PLTE =
	ChunkFlags({
		Chunk::bKGD, Chunk::hIST, Chunk::tRNS
	});

	static constexpr ChunkFlags before_IDAT =
	 ~ ChunkFlags({
		Chunk::IDAT, Chunk::IEND, Chunk::fcTL, Chunk::fdAT, Chunk::tIME, Chunk::iTXt, Chunk::tEXt, Chunk::zTXt
	});

	// 順序チェックとフラグの追加（「IDATの連続」「colotype=3の場合PLTE必須」「color_type=0,4の場合PLTE禁止」を除く）
	// https://www.w3.org/TR/png-3/#table53
	bool check_chunk_order_and_set (ChunkFlags& chunk_flags, const Chunk chunk) {
		// about IHDR
		if (chunk == Chunk::IHDR) {
			if (chunk_flags.any_set()) return false;
		} else {
			if (chunk_flags.is_clear(Chunk::IHDR)) return false;
		}
		// multiple disallowed
		if (multiple_disallowed.is_set(chunk)) {
			if (chunk_flags.is_set(chunk)) return false;
		}
		// before PLTE
		if (before_PLTE.is_set(chunk)) {
			if (chunk_flags.is_set(Chunk::PLTE)) return false;
		}
		// after PLTE
		if (chunk == Chunk::PLTE) {
			if ((chunk_flags & after_PLTE).any_set()) return false;
		}
		// before IDAT
		if (before_IDAT.is_set(chunk)) {
			if (chunk_flags.is_set(Chunk::IDAT)) return false;
		}
		// after IDAT
		if (chunk == Chunk::fcTL) {
			if (chunk_flags.is_set(Chunk::fcTL)) {
				if (chunk_flags.is_clear(Chunk::IDAT)) return false;
			}
		} else if (chunk == Chunk::IEND || chunk == Chunk::fdAT) {
			if (chunk_flags.is_clear(Chunk::IDAT)) return false;
		}
		// about IEND
		if (chunk_flags.is_set(Chunk::IEND)) return false;
		chunk_flags.set(chunk);
		return true;
	}

	bool read_IHDR (
		vector<u8>::const_iterator& itr,
		const u32 length,
		u32& W,
		u32& H,
		u8& bit_depth,
		u8& color_type,
		u8& interlace_method,
		SimpleInflate& inflater,
		vector<u8>& filtered_stream
	) {
		if (length != IHDR_SIZE) return false;
		W = readBE<u32>(itr);
		H = readBE<u32>(itr);
		bit_depth = *itr++;
		color_type = *itr++;
		u8 compression_method = *itr++;
		u8 filter_method = *itr++;
		interlace_method = *itr++;

		if (W == 0 || H == 0) return false; // https://www.w3.org/TR/png/#11IHDR
		if (W > FOUR_BYTE_LIMIT || H > FOUR_BYTE_LIMIT) return false; // https://www.w3.org/TR/png/#dfn-png-four-byte-unsigned-integer

		// https://www.w3.org/TR/png/#table111
		if (bit_depth == 1 || bit_depth == 2 || bit_depth == 4) {
			if (color_type != 0 && color_type != 3) return false;
		} else if (bit_depth == 8) {
			if (color_type != 0 && color_type != 2 && color_type != 3 && color_type != 4 && color_type != 6) return false;
		} else if (bit_depth == 16) {
			if (color_type != 0 && color_type != 2 && color_type != 4 && color_type != 6) return false;
		} else {
			return false;
		}

		if (compression_method) return false; // https://www.w3.org/TR/png/#10CompressionCM0
		if (filter_method) return false; // https://www.w3.org/TR/png/#9FtIntro
		if (interlace_method > 1) return false; // https://www.w3.org/TR/png/#8InterlaceMethods

		size_t filtered_size = 0;

		for (u8 i = (interlace_method ? 0 : 7); i < (interlace_method ? 7 : 8); ++i) {
			filtered_size += calc_filtered_size(
				calc_pixel_count(H, adam7_offsets[i], adam7_deltas[i]),
				calc_pixel_count(W, adam7_offsets[i + 1], adam7_deltas[i + 1]),
				bit_depth,
				color_type
			);
		}

		filtered_stream.resize(filtered_size);
		inflater.set_output(filtered_stream.data(), filtered_size);

		return true;
	}

	bool read_IDAT (
		vector<u8>::const_iterator& itr,
		const u32 length,
		SimpleInflate& inflater
	) {
		if (!inflater.feed_input(const_cast<u8*>(&*itr), length)) return false;
		itr += length;
		return true;
	}

	bool read_PLTE (
		vector<u8>::const_iterator& itr,
		const u32 length,
		array<RGBA8, 256>& palette_base,
		span<const RGBA8>& palette,
		u8 bit_depth
	) {
		if (length > u32((1 << bit_depth) * 3) || length % 3 > 0) return false;
		u16 palette_size = length / 3;
		for (u16 i = 0; i < palette_size; ++i) {
			palette_base[i].R = *itr++;
			palette_base[i].G = *itr++;
			palette_base[i].B = *itr++;
		}
		palette = span<const RGBA8>(palette_base.data(), palette_size);
		return true;
	}

	bool read_IEND (
		vector<u8>::const_iterator& itr,
		const u32 length
	) {
		if (length != IEND_SIZE) return false;
		itr += length;
		return true;
	}


	void write_IHDR (
		vector<u8>::iterator& itr,
		const u32 W,
		const u32 H,
		const u8 bit_depth,
		const u8 color_type
	) {
		writeBE<u32>(itr, IHDR_SIZE);
		*itr++ = 'I';
		*itr++ = 'H';
		*itr++ = 'D';
		*itr++ = 'R';
		const u8* ptr = &(*itr);
		writeBE<u32>(itr, W);
		writeBE<u32>(itr, H);
		*itr++ = bit_depth;
		*itr++ = color_type;
		*itr++ = 0;
		*itr++ = 0;
		*itr++ = 0;
		u32 crc = crc32_z(IHDR_crc, ptr, IHDR_SIZE);
		writeBE<u32>(itr, crc);
	}

	// Deflate済みデータは先に書き込まれている必要がある
	void write_IDAT (vector<u8>::iterator& itr, const size_t IDAT_size) {
		writeBE<u32>(itr, IDAT_size);
		*itr++ = 'I';
		*itr++ = 'D';
		*itr++ = 'A';
		*itr++ = 'T';
		u32 crc = crc32_z(IDAT_crc, &*itr, IDAT_size);
		itr += IDAT_size;
		writeBE<u32>(itr, crc);
	}

	void write_IEND (vector<u8>::iterator& itr) {
		writeBE<u32>(itr, IEND_SIZE);
		*itr++ = 'I';
		*itr++ = 'E';
		*itr++ = 'N';
		*itr++ = 'D';
		writeBE<u32>(itr, IEND_crc);
	}


	// a:left b:above c:upperleft
	static u8 paeth_predictor (const u8 c, const u8 b, const u8 a) {
		short pb = a, pa = b, pc;
		pa -= c; pb -= c; pc = pa + pb;
		pa = abs(pa); pb = abs(pb); pc = abs(pc);
		if (pa <= pb && pa <= pc) return a;
		if (pb <= pc) return b;
		return c;
	}

	void uf_Sub (span<u8> scanline, u8 offset) {
		auto aitr = scanline.begin();
		auto xitr = scanline.begin() + offset;
		while (xitr < scanline.end()) {
			*xitr++ += *aitr++;
		}
	}

	void uf_Up (span<const u8> scanline_up, span<u8> scanline) {
		auto bitr = scanline_up.begin();
		auto xitr = scanline.begin();
		while (xitr < scanline.end()) {
			*xitr++ += *bitr++;
		}
	}

	void uf_Ave (span<const u8> scanline_up, span<u8> scanline, u8 offset) {
		auto bitr = scanline_up.begin();
		auto aitr = scanline.begin();
		auto xitr = scanline.begin();
		while (offset--) {
			*xitr++ += *bitr++ >> 1;
		}
		while (xitr < scanline.end()) {
			*xitr++ += static_cast<u8>((static_cast<u16>(*bitr++) + static_cast<u16>(*aitr++)) >> 1);
		}
	}
	void uf_Ave (span<u8> scanline, u8 offset) {
		auto aitr = scanline.begin();
		auto xitr = scanline.begin() + offset;
		while (xitr < scanline.end()) {
			*xitr++ += *aitr++ >> 1;
		}
	}

	void uf_Paeth (span<const u8> scanline_up, span<u8> scanline, u8 offset) {
		auto citr = scanline_up.begin();
		auto bitr = scanline_up.begin();
		auto aitr = scanline.begin();
		auto xitr = scanline.begin();
		while (offset--) {
			*xitr++ += *bitr++;
		}
		while (xitr < scanline.end()) {
			*xitr++ += paeth_predictor(*citr++, *bitr++, *aitr++);
		}
	}

	vector<u8> f_None (span<const u8> scanline) {
		vector<u8> filtered(scanline.size());
		copy(scanline.begin(), scanline.end(), filtered.begin());
		return filtered;
	}

	vector<u8> f_Sub (span<const u8> scanline, u8 offset) {
		vector<u8> filtered(scanline.size());
		copy(scanline.begin(), scanline.end(), filtered.begin());
		auto aitr = scanline.begin();
		auto xitr = filtered.begin() + offset;
		while (xitr < filtered.end()) {
			*xitr++ -= *aitr++;
		}
		return filtered;
	}

	vector<u8> f_Up (span<const u8> scanline, span<const u8> scanline_up) {
		vector<u8> filtered(scanline.size());
		copy(scanline.begin(), scanline.end(), filtered.begin());
		auto bitr = scanline_up.begin();
		auto xitr = filtered.begin();
		while (xitr < filtered.end()) {
			*xitr++ -= *bitr++;
		}
		return filtered;
	}

	vector<u8> f_Ave (span<const u8> scanline, span<const u8> scanline_up, u8 offset) {
		vector<u8> filtered(scanline.size());
		copy(scanline.begin(), scanline.end(), filtered.begin());
		auto bitr = scanline_up.begin();
		auto aitr = scanline.begin();
		auto xitr = filtered.begin();
		while (offset--) {
			*xitr++ -= *bitr++ >> 1;
		}
		while (xitr < filtered.end()) {
			*xitr++ -= static_cast<u8>((static_cast<u16>(*bitr++) + static_cast<u16>(*aitr++)) >> 1);
		}
		return filtered;
	}

	vector<u8> f_Paeth (span<const u8> scanline, span<const u8> scanline_up, u8 offset) {
		vector<u8> filtered(scanline.size());
		copy(scanline.begin(), scanline.end(), filtered.begin());
		auto citr = scanline_up.begin();
		auto bitr = scanline_up.begin();
		auto aitr = scanline.begin();
		auto xitr = filtered.begin();
		while (offset--) {
			*xitr++ -= *bitr++;
		}
		while (xitr < filtered.end()) {
			*xitr++ -= paeth_predictor(*citr++, *bitr++, *aitr++);
		}
		return filtered;
	}

	bool unfilterer (
		vector<u8>::iterator itr,
		vector<span<const u8>>& scanlines,
		const size_t offset,
		const size_t scanline_size,
		const u32 height
	) {
		if (height == 0 || scanline_size == 0) return true;
		scanlines.clear();
		scanlines.reserve(height);
		for (u32 h = 0; h < height; ++h) {
			u8 filter_type = *itr;
			itr += FILTER_TYPE_SIZE;
			span<u8> scanline(itr, scanline_size);
			itr += scanline_size;
			switch (filter_type) {
				case 0: break; // None
				case 1: { // Sub
					uf_Sub(scanline, offset);
					break;
				}
				case 2: { // Up
					if (h == 0) break;
					uf_Up(scanlines[h - 1], scanline);
					break;
				}
				case 3: { // Average
					if (h == 0) {
						uf_Ave(scanline, offset);
					} else {
						uf_Ave(scanlines[h - 1], scanline, offset);
					}
					break;
				}
				case 4: { // Paeth
					if (h == 0) {
						uf_Sub(scanline, offset);
					} else {
						uf_Paeth(scanlines[h - 1], scanline, offset);
					}
					break;
				}
			}
			scanlines.push_back(scanline);
		}
		return true;
	}

	u64 sum_abs (const vector<u8>& vec) {
		u64 res = 0;
		for (const i8 i : vec) res += abs(i);
		return res;
	}

	void filterer (
		vector<u8>::iterator itr, // filtered_stream.end()
		const size_t offset,
		const size_t scanline_size,
		const u32 height
	) {
		span<const u8> scanline, scanline_up;
		scanline = span<const u8>(itr - scanline_size, scanline_size);
		array<vector<u8>, 5> filtered_array;
		for (u32 h = height; h-- > 0;) {
			itr -= scanline_size;
			if (h > 0) scanline_up = span<const u8>(itr - FILTER_TYPE_SIZE - scanline_size, scanline_size);
			filtered_array[0] = f_None(scanline);
			u8 best_filter = 0;
			u64 best_score = sum_abs(filtered_array[0]);
			for (u8 i = 1; i < 5; ++i) {
				switch (i) {
					case 1: {
						filtered_array[i] = f_Sub(scanline, offset);
						break;
					}
					case 2: {
						if (h == 0) continue;
						filtered_array[i] = f_Up(scanline, scanline_up);
						break;
					}
					case 3: {
						if (h == 0) continue;
						filtered_array[i] = f_Ave(scanline, scanline_up, offset);
						break;
					}
					case 4: {
						if (h == 0) continue;
						filtered_array[i] = f_Paeth(scanline, scanline_up, offset);
						break;
					}
				}
				u64 score = sum_abs(filtered_array[i]);
				if (score < best_score) {
					best_score = score;
					best_filter = i;
				}
			}
			copy(filtered_array[best_filter].begin(), filtered_array[best_filter].end(), itr);
			itr -= FILTER_TYPE_SIZE;
			*itr = best_filter;
			scanline = scanline_up;
		}
	}


	bool read_indexed_scanlines (
		const vector<span<const u8>>& scanlines,
		const span<const RGBA8> palette,
		const u32 y0 = 0,
		const u32 dy = 1,
		const u32 x0 = 0,
		const u32 dx = 1
	) {
		u32 y = y0;
		for (const auto& scanline : scanlines) {
			auto data_itr = data[y].begin() + x0;
			for (const u8 index : scanline) {
				if (index >= palette.size()) return false;
				*data_itr = palette[index];
				data_itr += dx;
			}
			y += dy;
		}
		return true;
	}

	template<u8 bit_depth> requires (is_png_subbyte<bit_depth>)
	bool read_indexed_scanline (
		const span<const u8> scanline,
		const span<const RGBA8> palette,
		span<Pixel> data_row,
		const u32 x0 = 0,
		const u32 dx = 1
	) {
		const u32 width = data_row.size();
		auto data_itr = data_row.begin() + x0;
		u32 pixel_count = calc_pixel_count(width, x0, dx);
		u8 index;
		if constexpr (bit_depth == 1) {
			for (const u8 byte : scanline) {
				for (u8 shift = 8; shift > 0;) {
					shift -= 1;
					if (pixel_count-- == 0) return true;
					index = (byte >> shift) & 1;
					if (index >= palette.size()) return false;
					*data_itr = palette[index];
					data_itr += dx;
				}
			}
		} else if constexpr (bit_depth == 2) {
			for (const u8 byte : scanline) {
				for (u8 shift = 8; shift > 0;) {
					shift -= 2;
					if (pixel_count-- == 0) return true;
					index = (byte >> shift) & 3;
					if (index >= palette.size()) return false;
					*data_itr = palette[index];
					data_itr += dx;
				}
			}
		} else if constexpr (bit_depth == 4) {
			for (const u8 byte : scanline) {
				for (u8 shift = 8; shift > 0;) {
					shift -= 4;
					if (pixel_count-- == 0) return true;
					index = (byte >> shift) & 15;
					if (index >= palette.size()) return false;
					*data_itr = palette[index];
					data_itr += dx;
				}
			}
		}
		return true;
	}

	template<u8 bit_depth> requires (is_png_subbyte<bit_depth>)
	bool read_indexed_scanlines (
		const vector<span<const u8>>& scanlines,
		const span<const RGBA8> palette,
		const u32 y0 = 0,
		const u32 dy = 1,
		const u32 x0 = 0,
		const u32 dx = 1
	) {
		u32 y = y0;
		for (const auto& scanline : scanlines) {
			if (!read_indexed_scanline<bit_depth>(scanline, palette, data[y], x0, dx)) return false;
			y += dy;
		}
		return true;
	}

	bool read_indexed_data (
		vector<u8>::iterator itr,
		const u32 H,
		const u32 W,
		const u8 bit_depth,
		const u8 interlace_method,
		const span<const RGBA8> palette
	) {
		for (u8 i = (interlace_method ? 0 : 7); i < (interlace_method ? 7 : 8); ++i) {
			const u32 y0 = adam7_offsets[i];
			const u32 dy = adam7_deltas[i];
			const u32 x0 = adam7_offsets[i + 1];
			const u32 dx = adam7_deltas[i + 1];
			const u32 pixel_count_y = calc_pixel_count(H, y0, dy);
			const u32 pixel_count_x = calc_pixel_count(W, x0, dx);
			const size_t scanline_size = calc_scanline_size(pixel_count_x, bit_depth, 3);
			if (calc_filtered_size(pixel_count_y, pixel_count_x, bit_depth, 3) == 0) continue;
			vector<span<const u8>> scanlines;
			if (!unfilterer(itr, scanlines, 1, scanline_size, pixel_count_y)) return false;
			switch (bit_depth) {
			case 1: if (!read_indexed_scanlines<1>(scanlines, palette, y0, dy, x0, dx)) return false; break;
			case 2: if (!read_indexed_scanlines<2>(scanlines, palette, y0, dy, x0, dx)) return false; break;
			case 4: if (!read_indexed_scanlines<4>(scanlines, palette, y0, dy, x0, dx)) return false; break;
			case 8: if (!read_indexed_scanlines   (scanlines, palette, y0, dy, x0, dx)) return false; break;
			}
		}
		return true;
	}

	template<pixel_type Pixel_tmp>
	Pixel_tmp read_pixel (span<const u8>::const_iterator& itr) {
		using U = typename pixel_traits<Pixel_tmp>::channel_type;
		if constexpr (pixel_traits<Pixel_tmp>::rgb) {
			U R = readBE<U>(itr);
			U G = readBE<U>(itr);
			U B = readBE<U>(itr);
			if constexpr (pixel_traits<Pixel_tmp>::alpha) {
				U A = readBE<U>(itr);
				return Pixel_tmp(R, G, B, A);
			} else {
				return Pixel_tmp(R, G, B);
			}
		} else {
			U Y = readBE<U>(itr);
			if constexpr (pixel_traits<Pixel_tmp>::alpha) {
				U A = readBE<U>(itr);
				return Pixel_tmp(Y, A);
			} else {
				return Pixel_tmp(Y);
			}
		}
	}

	template<pixel_type Pixel_tmp>
	void read_direct_scanlines (
		const vector<span<const u8>>& scanlines,
		const u32 y0 = 0,
		const u32 dy = 1,
		const u32 x0 = 0,
		const u32 dx = 1
	) {
		u32 y = y0;
		for (const auto& scanline : scanlines) {
			auto data_itr = data[y].begin() + x0;
			auto itr = scanline.begin();
			while (itr < scanline.end()) {
				*data_itr = read_pixel<Pixel_tmp>(itr);
				data_itr += dx;
			}
			y += dy;
		}
	}

	// チャンネル数が1のGreyのみが対象
	template<u8 bit_depth> requires (is_png_subbyte<bit_depth>)
	void read_direct_scanline (
		const span<const u8> scanline,
		span<Pixel> data_row,
		const u32 x0 = 0,
		const u32 dx = 1
	) {
		const u32 width = data_row.size();
		auto data_itr = data_row.begin() + x0;
		u32 pixel_count = calc_pixel_count(width, x0, dx);
		T Y;
		if constexpr (bit_depth == 1) {
			for (const u8 byte : scanline) {
				for (u8 shift = 8; shift > 0;) {
					shift -= 1;
					if (pixel_count-- == 0) return;
					Y = convert_bitdepth<T, bit_depth>((byte >> shift) & 1);
					*data_itr = Pixel(Y);
					data_itr += dx;
				}
			}
		} else if constexpr (bit_depth == 2) {
			for (const u8 byte : scanline) {
				for (u8 shift = 8; shift > 0;) {
					shift -= 2;
					if (pixel_count-- == 0) return;
					Y = convert_bitdepth<T, bit_depth>((byte >> shift) & 3);
					*data_itr = Pixel(Y);
					data_itr += dx;
				}
			}
		} else if constexpr (bit_depth == 4) {
			for (const u8 byte : scanline) {
				for (u8 shift = 8; shift > 0;) {
					shift -= 4;
					if (pixel_count-- == 0) return;
					Y = convert_bitdepth<T, bit_depth>((byte >> shift) & 15);
					*data_itr = Pixel(Y);
					data_itr += dx;
				}
			}
		}
	}

	template<u8 bit_depth> requires (is_png_subbyte<bit_depth>)
	void read_direct_scanlines (
		const vector<span<const u8>>& scanlines,
		const u32 y0 = 0,
		const u32 dy = 1,
		const u32 x0 = 0,
		const u32 dx = 1
	) {
		u32 y = y0;
		for (const auto& scanline : scanlines) {
			read_direct_scanline<bit_depth>(scanline, data[y], x0, dx);
			y += dy;
		}
	}

	bool read_direct_data (
		vector<u8>::iterator itr,
		const u32 H,
		const u32 W,
		const u8 bit_depth,
		const u8 color_type,
		const u8 interlace_method
	) {
		const u8 offset = bit_depth < 8 ? 1 : colortype2channel[color_type] * bit_depth / 8;
		for (u8 i = (interlace_method ? 0 : 7); i < (interlace_method ? 7 : 8); ++i) {
			const u32 y0 = adam7_offsets[i];
			const u32 dy = adam7_deltas[i];
			const u32 x0 = adam7_offsets[i + 1];
			const u32 dx = adam7_deltas[i + 1];
			const u32 pixel_count_y = calc_pixel_count(H, y0, dy);
			const u32 pixel_count_x = calc_pixel_count(W, x0, dx);
			const size_t scanline_size = calc_scanline_size(pixel_count_x, bit_depth, color_type);
			if (calc_filtered_size(pixel_count_y, pixel_count_x, bit_depth, color_type) == 0) continue;
			vector<span<const u8>> scanlines;
			if (!unfilterer(itr, scanlines, offset, scanline_size, pixel_count_y)) return false;
			switch (bit_depth) {
			case 1: read_direct_scanlines<1>(scanlines, y0, dy, x0, dx); break;
			case 2: read_direct_scanlines<2>(scanlines, y0, dy, x0, dx); break;
			case 4: read_direct_scanlines<4>(scanlines, y0, dy, x0, dx); break;
			case 8: {
				switch (color_type) {
				case 0: read_direct_scanlines<Grey<u8>>(scanlines, y0, dy, x0, dx); break;
				case 2: read_direct_scanlines<RGB<u8>>(scanlines, y0, dy, x0, dx); break;
				case 4: read_direct_scanlines<GreyAlpha<u8>>(scanlines, y0, dy, x0, dx); break;
				case 6: read_direct_scanlines<RGBA<u8>>(scanlines, y0, dy, x0, dx); break;
				}
				break;
			}
			case 16: {
				switch (color_type) {
				case 0: read_direct_scanlines<Grey<u16>>(scanlines, y0, dy, x0, dx); break;
				case 2: read_direct_scanlines<RGB<u16>>(scanlines, y0, dy, x0, dx); break;
				case 4: read_direct_scanlines<GreyAlpha<u16>>(scanlines, y0, dy, x0, dx); break;
				case 6: read_direct_scanlines<RGBA<u16>>(scanlines, y0, dy, x0, dx); break;
				}
				break;
			}
			}
		}
		return true;
	}

	void write_pixel (vector<u8>::iterator& itr, const Pixel& pixel) {
		using U = typename pixel_traits<Pixel>::channel_type;
		if constexpr (pixel_traits<Pixel>::rgb) {
			writeBE<U>(itr, pixel.R);
			writeBE<U>(itr, pixel.G);
			writeBE<U>(itr, pixel.B);
			if constexpr (pixel_traits<Pixel>::alpha) {
				writeBE<U>(itr, pixel.A);
			}
		} else {
			writeBE<U>(itr, pixel.Y);
			if constexpr (pixel_traits<Pixel>::alpha) {
				writeBE<U>(itr, pixel.A);
			}
		}
	}

	void write_data (vector<u8>::iterator itr) {
		for (u32 h = 0; h < data.height(); ++h) {
			itr += FILTER_TYPE_SIZE;
			for (const Pixel& pixel : data[h]) {
				write_pixel(itr, pixel);
			}
		}
	}


	void deflate_RLE (
		vector<u8>& src,
		vector<u8>& dest,
		const size_t head_offset,
		const size_t tail_offset,
		u8 level = 7
	) {
		size_t dest_size = head_offset + compressBound(src.size()) + tail_offset;
		dest.resize(dest_size);
		z_stream z; z.zalloc = Z_NULL; z.zfree = Z_NULL; z.opaque = Z_NULL;
		if (deflateInit2(
			&z,
			level, // 圧縮レベル
			Z_DEFLATED,
			15, // ウィンドウサイズは最大
			8, // 手元だと 8 が最も良かった
			Z_RLE // ランレングス圧縮
		) != Z_OK) return;
		z.next_in = src.data();
		z.avail_in = src.size();
		z.next_out = dest.data() + head_offset;
		z.avail_out = dest.size() - head_offset - tail_offset;
		i32 ret;
		do{
			ret = deflate(&z, Z_FINISH);
			if (ret != Z_OK && ret != Z_STREAM_END) return;
		} while (ret != Z_STREAM_END);
		deflateEnd(&z);
		dest.resize(head_offset + z.total_out + tail_offset);
	}

};


#endif
