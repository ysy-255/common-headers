#ifndef SIMPLEZLIB_HPP
#define SIMPLEZLIB_HPP

#include <zlib.h>

#include "int.hpp"


class SimpleInflate{
	z_stream z{};
	bool active = false;
	bool finished = false;
public:
	SimpleInflate() {
		active = (inflateInit(&z) == Z_OK);
	}
	explicit operator bool() const{
		return active;
	}
	~SimpleInflate() {
		if (active) {
			inflateEnd(&z);
		}
	}
	SimpleInflate (const SimpleInflate&) = delete;
	SimpleInflate& operator= (const SimpleInflate&) = delete;

	void set_output (u8* dst, const u32 size) {
		z.next_out = dst;
		z.avail_out = size;
	}
	bool feed_input (u8* src, const u32 length) {
		z.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(src));
		z.avail_in = length;
		while (z.avail_in > 0) {
			const i32 ret = inflate(&z, Z_NO_FLUSH);
			if (ret == Z_STREAM_END) {
				finished = true;
				break;
			}
			if (ret != Z_OK) {
				return false;
			}
		}
		return true;
	}
	bool reached_end() const{ // Z_STREAMの終端に到達した
		return finished;
	}
	bool input_completed() const{ // 渡した入力をすべて消費した
		return z.avail_in == 0;
	}
	bool output_completed() const{ // 出力バッファがいっぱいになった
		return z.avail_out == 0;
	}
	bool all_completed() const{
		return finished && input_completed() && output_completed();
	}
};


#endif
