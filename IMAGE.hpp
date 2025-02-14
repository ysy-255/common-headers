#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <bits/stdint-uintn.h>
#include <vector>

template<typename T>
class RGB;

template<typename T>
class RGBA;


template<typename T>
class IMAGE_RGB;

template<typename T>
class IMAGE_RGBA;


template<typename T>
class RGB{
	public:
	T R = 0;
	T G = 0;
	T B = 0;
	RGB& operator=(const RGBA<T> & other){
		R = other.R;
		G = other.G;
		B = other.B;
		return *this;
	}
};

template<typename T>
class RGBA{
	public:
	T R = 0;
	T G = 0;
	T B = 0;
	T A = 0xFF;
	RGBA& operator=(const RGB<T> & other){
		R = other.R;
		G = other.G;
		B = other.B;
		A = 0xFF;
		return *this;
	}
};


template<typename T>
class IMAGE_RGB{
	public:
	friend class IMAGE_RGBA<T>;

	IMAGE_RGB(){}
	IMAGE_RGB(const uint32_t W, const uint32_t H) :
		width(W), height(H),
		data(std::vector<std::vector<RGB<T>>>(H, std::vector<RGB<T>>(W))) {}
	IMAGE_RGB(const IMAGE_RGBA<T> & img) : width(img.width), height(img.height){
		data = std::vector<std::vector<RGB<T>>>(height, std::vector<RGB<T>>(width));
		for(uint32_t h = 0; h < height; ++h){
			for(uint32_t w = 0; w < width; ++w){
				data[h][w] = img.data[h][w];
			}
		}
	}

	inline std::vector<RGB<T>> & operator[](size_t h){
		return data[h];
	}

	uint32_t width;
	uint32_t height;

	protected:

	std::vector<std::vector<RGB<T>>> data;
};



template<typename T>
class IMAGE_RGBA{
	public:
	friend class IMAGE_RGB<T>;

	IMAGE_RGBA(){}
	IMAGE_RGBA(const uint32_t W, const uint32_t H) :
		width(W), height(H),
		data(std::vector<std::vector<RGBA<T>>>(H, std::vector<RGBA<T>>(W))) {}
	IMAGE_RGBA(const IMAGE_RGB<T> & img) : width(img.width), height(img.height){
		data = std::vector<std::vector<RGBA<T>>>(height, std::vector<RGBA<T>>(width));
		for(uint32_t h = 0; h < height; ++h){
			for(uint32_t w = 0; w < width; ++w){
				data[h][w] = img.data[h][w];
			}
		}
	}

	inline std::vector<RGBA<T>> & operator[](size_t h){
		return data[h];
	}

	uint32_t width;
	uint32_t height;

	protected:

	std::vector<std::vector<RGBA<T>>> data;
};

#endif
