#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vector>

#include "int.hpp"

struct RGB8;
struct RGBA8;
class Image_RGB8;
class Image_RGBA8;

struct RGB8{
	u8 R = 0;
	u8 G = 0;
	u8 B = 0;
	RGB8& operator=(const RGBA8 & other);
};

struct RGBA8{
	u8 R = 0;
	u8 G = 0;
	u8 B = 0;
	u8 A = U8MAX;
	RGBA8& operator=(const RGB8 & other);
};

RGB8& RGB8::operator=(const RGBA8 & other){
	R = other.R;
	G = other.G;
	B = other.B;
	return *this;
}

RGBA8& RGBA8::operator=(const RGB8 & other){
	R = other.R;
	G = other.G;
	B = other.B;
	A = U8MAX;
	return *this;
}

class Image_RGB8{
public:
	friend class Image_RGBA8;

	size_t H, W;

	Image_RGB8() = default;
	Image_RGB8(size_t Height, size_t Width) : H(Height), W(Width), data(H, std::vector<RGB8>(W)) {}
	Image_RGB8(const Image_RGBA8 & img);

	Image_RGB8 & operator=(const Image_RGBA8 & other);

	std::vector<RGB8> & operator[](const size_t h){ return data[h]; }


protected:
	std::vector<std::vector<RGB8>> data;
};

class Image_RGBA8{
public:
	friend class Image_RGB8;

	size_t H, W;

	Image_RGBA8() = default;
	Image_RGBA8(size_t Height, size_t Width) : H(Height), W(Width), data(H, std::vector<RGBA8>(W)) {}
	Image_RGBA8(const Image_RGB8 & img);

	Image_RGBA8 & operator=(const Image_RGB8 & other);

	std::vector<RGBA8> & operator[](size_t h){ return data[h]; }


protected:
	std::vector<std::vector<RGBA8>> data;
};

Image_RGB8::Image_RGB8(const Image_RGBA8 & img) : H(img.H), W(img.W), data(H, std::vector<RGB8>(W)){
	for(size_t h = 0; h < H; ++h){
		for(size_t w = 0; w < W; ++w){
			data[h][w] = img.data[h][w];
		}
	}
}
Image_RGB8 & Image_RGB8::operator=(const Image_RGBA8 & other){
	H = other.H;
	W = other.W;
	data = std::vector<std::vector<RGB8>>(H, std::vector<RGB8>(W));
	for(size_t h = 0; h < H; ++h){
		for(size_t w = 0; w < W; ++w){
			data[h][w] = other.data[h][w];
		}
	}
	return *this;
}
Image_RGBA8::Image_RGBA8(const Image_RGB8 & img) : H(img.H), W(img.W), data(H, std::vector<RGBA8>(W)){
	for(size_t h = 0; h < H; ++h){
		for(size_t w = 0; w < W; ++w){
			data[h][w] = img.data[h][w];
		}
	}
}
Image_RGBA8 & Image_RGBA8::operator=(const Image_RGB8 & other){
	H = other.H;
	W = other.W;
	data = std::vector<std::vector<RGBA8>>(H, std::vector<RGBA8>(W));
	for(size_t h = 0; h < H; ++h){
		for(size_t w = 0; w < W; ++w){
			data[h][w] = other.data[h][w];
		}
	}
	return *this;
}

#endif
