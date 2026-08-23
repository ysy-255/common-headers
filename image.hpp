#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <concepts>
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <vector>
#include <span>

#include "int.hpp"

template<typename T>
using limits = std::numeric_limits<T>;
using std::vector;
using std::span;

template<class T>
concept unsigned_channel =
	std::unsigned_integral<T> &&
	!std::same_as<T, bool> &&
	std::same_as<T, std::remove_cv_t<T>>;

template<unsigned_channel T> struct Grey;
template<unsigned_channel T> struct GreyAlpha;
template<unsigned_channel T> struct RGB;
template<unsigned_channel T> struct RGBA;

using RGB8  = RGB<u8>;
using RGBA8 = RGBA<u8>;

template<class T>
struct pixel_traits;

template<unsigned_channel T>
struct pixel_traits<Grey<T>> {
	using channel_type = T;
	static constexpr bool rgb = false;
	static constexpr bool alpha = false;
};

template<unsigned_channel T>
struct pixel_traits<GreyAlpha<T>> {
	using channel_type = T;
	static constexpr bool rgb = false;
	static constexpr bool alpha = true;
};

template<unsigned_channel T>
struct pixel_traits<RGB<T>> {
	using channel_type = T;
	static constexpr bool rgb = true;
	static constexpr bool alpha = false;
};

template<unsigned_channel T>
struct pixel_traits<RGBA<T>> {
	using channel_type = T;
	static constexpr bool rgb = true;
	static constexpr bool alpha = true;
};

template<class T>
concept pixel_type = requires {
	typename pixel_traits<T>::channel_type;
	pixel_traits<T>::rgb;
	pixel_traits<T>::alpha;
};

template<class T>
concept rgb_family =
	pixel_type<T> &&
	pixel_traits<T>::rgb;

template<pixel_type Pixel>
using pixel_channel_t = typename pixel_traits<Pixel>::channel_type;

// Rec. 601
template<rgb_family Pixel>
inline constexpr pixel_channel_t<Pixel> rgb_to_grey (const Pixel& pixel) {
	return static_cast<pixel_channel_t<Pixel>>(
		pixel.R * 0.299 + pixel.G * 0.587 + pixel.B * 0.114
	);
}

template<unsigned_channel To, unsigned_channel From>
	requires (
		(limits<To>::digits % limits<From>::digits == 0) ||
		(limits<To>::digits < limits<From>::digits)
	)
inline constexpr To convert_bitdepth (const From from) {
	constexpr auto from_bits = limits<From>::digits;
	constexpr auto to_bits = limits<To>::digits;
	if constexpr (from_bits == to_bits) {
		return static_cast<To>(from);
	} else if constexpr (from_bits < to_bits) {
		constexpr To multiplier = limits<To>::max() / limits<From>::max();
		return static_cast<To>(multiplier * static_cast<To>(from));
	} else {
		return static_cast<To>(from >> (from_bits - to_bits));
	}
}
template<unsigned_channel To, u8 from_digits>
	requires (
		(from_digits == 1 || from_digits == 2 || from_digits == 4) &&
		(limits<To>::digits % from_digits == 0)
	)
inline constexpr To convert_bitdepth (const u8 from) {
	constexpr static To multiplier = limits<To>::max() / ((1 << from_digits) - 1);
	return static_cast<To>(multiplier * static_cast<To>(from));
}

template<unsigned_channel T>
struct Grey{
	T Y = 0;

	Grey() = default;
	Grey (T y) : Y(y) {}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, Grey<T>>)
	Grey (const Pixel& other) {
		if constexpr (pixel_traits<Pixel>::rgb) {
			Y = convert_bitdepth<T>(rgb_to_grey(other));
		} else {
			Y = convert_bitdepth<T>(other.Y);
		}
	}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, Grey<T>>)
	Grey& operator= (const Pixel& other) {
		if constexpr (pixel_traits<Pixel>::rgb) {
			Y = convert_bitdepth<T>(rgb_to_grey(other));
		} else {
			Y = convert_bitdepth<T>(other.Y);
		}
		return *this;
	}
};

template<unsigned_channel T>
struct GreyAlpha : Grey<T>{
	using Grey<T>::Y;
	T A = limits<T>::max();

	GreyAlpha() = default;
	GreyAlpha (T y)           : Grey<T>(y) {}
	GreyAlpha (T y, T a)      : Grey<T>(y), A(a) {}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, GreyAlpha<T>>)
	GreyAlpha (const Pixel& other) : Grey<T>(other) {
		if constexpr (pixel_traits<Pixel>::alpha) {
			A = convert_bitdepth<T>(other.A);
		} else {
			A = limits<T>::max();
		}
	}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, GreyAlpha<T>>)
	GreyAlpha& operator= (const Pixel& other) {
		Grey<T>::operator=(other);
		if constexpr (pixel_traits<Pixel>::alpha) {
			A = convert_bitdepth<T>(other.A);
		} else {
			A = limits<T>::max();
		}
		return *this;
	}
};

template<unsigned_channel T>
struct RGB{
	T R = 0;
	T G = 0;
	T B = 0;

	RGB() = default;
	RGB (T r, T g, T b) : R(r), G(g), B(b) {}
	RGB (T y)           : R(y), G(y), B(y) {}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, RGB<T>>)
	RGB (const Pixel& other) {
		if constexpr (rgb_family<Pixel>) {
			R = convert_bitdepth<T>(other.R);
			G = convert_bitdepth<T>(other.G);
			B = convert_bitdepth<T>(other.B);
		} else {
			R = G = B = convert_bitdepth<T>(other.Y);
		}
	}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, RGB<T>>)
	RGB& operator= (const Pixel& other) {
		if constexpr (rgb_family<Pixel>) {
			R = convert_bitdepth<T>(other.R);
			G = convert_bitdepth<T>(other.G);
			B = convert_bitdepth<T>(other.B);
		} else {
			R = G = B = convert_bitdepth<T>(other.Y);
		}
		return *this;
	}
};

template<unsigned_channel T>
struct RGBA : RGB<T>{
	using RGB<T>::R;
	using RGB<T>::G;
	using RGB<T>::B;
	T A = limits<T>::max();

	RGBA() = default;
	RGBA (T r, T g, T b)      : RGB<T>(r, g, b) {}
	RGBA (T r, T g, T b, T a) : RGB<T>(r, g, b), A(a) {}
	RGBA (T y)                : RGB<T>(y) {}
	RGBA (T y, T a)           : RGB<T>(y), A(a) {}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, RGBA<T>>)
	RGBA (const Pixel& other) : RGB<T>(other) {
		if constexpr (pixel_traits<Pixel>::alpha) {
			A = convert_bitdepth<T>(other.A);
		} else {
			A = limits<T>::max();
		}
	}

	template<pixel_type Pixel> requires (!std::same_as<Pixel, RGBA<T>>)
	RGBA& operator= (const Pixel& other) {
		RGB<T>::operator=(other);
		if constexpr (pixel_traits<Pixel>::alpha) {
			A = convert_bitdepth<T>(other.A);
		} else {
			A = limits<T>::max();
		}
		return *this;
	}
};

template<pixel_type Pixel>
class Image{
public:

	Image() = default;
	Image (u32 Height, u32 Width) : H(Height), W(Width), data(H * W) {}

	template<pixel_type OtherPixel> requires (!std::same_as<OtherPixel, Pixel>)
	Image (const Image<OtherPixel>& other) :
		H(other.H), W(other.W), data(H * W) {
		for (size_t i = 0; i < H * W; ++i) {
			data[i] = Pixel(other.data[i]);
		}
	}

	template<pixel_type OtherPixel> requires (!std::same_as<OtherPixel, Pixel>)
	Image& operator= (const Image<OtherPixel>& other) {
		H = other.H;
		W = other.W;
		data.resize(H * W);
		for (size_t i = 0; i < H * W; ++i) {
			data[i] = Pixel(other.data[i]);
		}
		return *this;
	}

	u32 height() const { return H; }
	u32 width() const { return W; }

	span<Pixel> operator[] (const u32 h) { return {data.data() + h * W, W}; }
	span<const Pixel> operator[] (const u32 h) const{ return {data.data() + h * W, W}; }

	Pixel& at (const u32 h, const u32 w) {
		if (h >= H || w >= W) throw std::out_of_range("Image::at");
		return data[h * W + w];
	}
	const Pixel& at (const u32 h, const u32 w) const {
		if (h >= H || w >= W) throw std::out_of_range("Image::at");
		return data[h * W + w];
	}

	vector<Pixel>::iterator begin() { return data.begin(); }
	vector<Pixel>::iterator end() { return data.end(); }
	vector<Pixel>::const_iterator begin() const { return data.begin(); }
	vector<Pixel>::const_iterator end() const { return data.end(); }

protected:
	template<pixel_type OtherPixel>
	friend class Image;

	size_t H = 0, W = 0;
	vector<Pixel> data;
};

#endif
