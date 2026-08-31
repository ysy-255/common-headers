#ifndef FILE_HPP
#define FILE_HPP

#include <bit>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>
#include <concepts>
#include <iterator>
#include <type_traits>

#include "int.hpp"

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::copy;
using std::reverse_copy;


inline vector<u8> readFile (const string& path) {
	ifstream file_ifstream(path, std::ios::binary | std::ios::ate);
	if (!file_ifstream.is_open()) return {};
	size_t file_size = file_ifstream.tellg();
	file_ifstream.seekg(0);
	vector<u8> result(file_size);
	file_ifstream.read(reinterpret_cast<char*>(result.data()), file_size);
	file_ifstream.close();
	return result;
}

inline void writeFile (const string& path, const vector<u8>& stream) {
	ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char*>(stream.data()), stream.size());
	out.close();
}


inline vector<string> getFileList (const string& folder_path) {
	vector<string> result;
	auto folder_files = std::filesystem::directory_iterator(folder_path);
	for (auto & f : folder_files) {
		if (f.is_regular_file()) {
			result.push_back(f.path().filename().string());
		}
	}
	return result;
}


template<typename T>
concept is_octet_type =
	std::same_as<std::remove_cv_t<T>, u8> ||
	std::same_as<std::remove_cv_t<T>, i8> ||
	std::same_as<std::remove_cv_t<T>, char>;

template<typename T>
concept is_arithmetic_not_bool =
	std::is_arithmetic_v<T> &&
	!std::same_as<T, bool> &&
	std::same_as<T, std::remove_cv_t<T>>;

template<typename T>
concept is_multibyte_arithmetic =
	is_arithmetic_not_bool<T> &&
	!is_octet_type<T>;

template<typename I>
concept octet_input_iterator =
	std::bidirectional_iterator<I> &&
	is_octet_type<std::iter_value_t<I>>;

template<typename O>
concept octet_output_iterator =
	std::output_iterator<O, u8>;

template<typename T, typename I> concept bufstream_read   = octet_input_iterator<I>  && is_arithmetic_not_bool<T>;
template<typename T, typename O> concept bufstream_write  = octet_output_iterator<O> && is_arithmetic_not_bool<T>;

template<typename T, typename I> concept bufstream_multibyte_read   = octet_input_iterator<I>  && is_multibyte_arithmetic<T>;
template<typename T, typename I> concept bufstream_singlebyte_read  = octet_input_iterator<I>  && is_octet_type<T>;
template<typename T, typename O> concept bufstream_multibyte_write  = octet_output_iterator<O> && is_multibyte_arithmetic<T>;
template<typename T, typename O> concept bufstream_singlebyte_write = octet_output_iterator<O> && is_octet_type<T>;

template<std::endian E>
consteval void check_endian_supported () {
	if constexpr (E != std::endian::native) {
		static_assert(
			std::endian::native == std::endian::little || 
			std::endian::native == std::endian::big,
			"Unsupported endianness"
		);
	}
}


template<typename T, std::endian E, typename II> requires (bufstream_multibyte_read<T, II>)
inline T readValue (II& itr) {
	check_endian_supported<E>();
	T res;
	u8* dst = reinterpret_cast<u8*>(&res);
	II last = itr;
	std::advance(last, sizeof(T));
	if constexpr (std::endian::native == E) {
		copy(itr, last, dst);
	} else {
		reverse_copy(itr, last, dst);
	}
	itr = last;
	return res;
}
template<typename T, std::endian E, typename II> requires (bufstream_singlebyte_read<T, II>)
inline T readValue (II& itr) {
	return static_cast<T>(*itr++);
}
template<typename T, typename II> requires (bufstream_multibyte_read<T, II>)
inline T readValue (II& itr, const bool is_little_endian) {
	if (is_little_endian) {
		return readValue<T, std::endian::little>(itr);
	} else {
		return readValue<T, std::endian::big>(itr);
	}
}
// [01][23][45][67] -> 0x01234567
template<typename T, typename II> requires (bufstream_multibyte_read<T, II>)
inline T readBE (II& itr) {
	return readValue<T, std::endian::big>(itr);
}
// [67][45][23][01] -> 0x01234567
template<typename T, typename II> requires (bufstream_multibyte_read<T, II>)
inline T readLE (II& itr) {
	return readValue<T, std::endian::little>(itr);
}
template<typename T, typename II> requires (bufstream_singlebyte_read<T, II>)
inline T readValue (II& itr, [[maybe_unused]] const bool is_little_endian = true) {
	return static_cast<T>(*itr++);
}
template<typename T, typename II> requires (bufstream_singlebyte_read<T, II>)
inline T readBE (II& itr) {
	return readValue<T>(itr);
}
template<typename T, typename II> requires (bufstream_singlebyte_read<T, II>)
inline T readLE (II& itr) {
	return readValue<T>(itr);
}


template<typename T, std::endian E, typename OI> requires (bufstream_multibyte_write<T, OI>)
inline void writeValue (OI& itr, const T value) {
	check_endian_supported<E>();
	const u8* src = reinterpret_cast<const u8*>(&value);
	if constexpr (std::endian::native == E) {
		itr = copy(src, src + sizeof(T), itr);
	} else {
		itr = reverse_copy(src, src + sizeof(T), itr);
	}
}
template<typename T, std::endian E, typename OI> requires (bufstream_singlebyte_write<T, OI>)
inline void writeValue (OI& itr, const T value) {
	*itr++ = static_cast<u8>(value);
}
template<typename T, typename OI> requires (bufstream_multibyte_write<T, OI>)
inline void writeValue (OI& itr, const T value, const bool is_little_endian) {
	if (is_little_endian) {
		return writeValue<T, std::endian::little>(itr, value);
	} else {
		return writeValue<T, std::endian::big>(itr, value);
	}
}
// 0x01234567 -> [01][23][45][67]
template<typename T, typename OI> requires (bufstream_multibyte_write<T, OI>)
inline void writeBE (OI& itr, const T value) {
	return writeValue<T, std::endian::big>(itr, value);
}
// 0x01234567 -> [67][45][23][01]
template<typename T, typename OI> requires (bufstream_multibyte_write<T, OI>)
inline void writeLE (OI& itr, const T value) {
	return writeValue<T, std::endian::little>(itr, value);
}
template<typename T, typename OI> requires (bufstream_singlebyte_write<T, OI>)
inline void writeValue (OI& itr, const T value, [[maybe_unused]] const bool is_little_endian = true) {
	*itr++ = static_cast<u8>(value);
}
template<typename T, typename OI> requires (bufstream_singlebyte_write<T, OI>)
inline void writeBE (OI& itr, const T value) {
	return writeValue<T>(itr, value);
}
template<typename T, typename OI> requires (bufstream_singlebyte_write<T, OI>)
inline void writeLE (OI& itr, const T value) {
	return writeValue<T>(itr, value);
}


template<typename II> requires (octet_input_iterator<II>)
inline std::string readString (II& itr, const size_t size) {
	II last = itr;
	std::advance(last, size);
	std::string res(itr, last);
	itr = last;
	return res;
}
template<typename II> requires (octet_input_iterator<II>)
inline vector<u8> readBytes (II& itr, const size_t size) {
	II last = itr;
	std::advance(last, size);
	vector<u8> res(itr, last);
	itr = last;
	return res;
}


template<typename OI> requires (octet_output_iterator<OI>)
inline void writeString (OI& itr, const string& str) {
	itr = std::copy(str.begin(), str.end(), itr);
}
template<typename OI> requires (octet_output_iterator<OI>)
inline void writeBytes (OI& itr, const vector<u8>& bytes) {
	itr = std::copy(bytes.begin(), bytes.end(), itr);
}


template<typename T, std::endian E, typename II> requires (bufstream_read<T, II>)
inline T peekValue (const II& itr) {
	check_endian_supported<E>();
	T res;
	u8* dst = reinterpret_cast<u8*>(&res);
	II last = itr;
	std::advance(last, sizeof(T));
	if constexpr (std::endian::native == E) {
		copy(itr, last, dst);
	} else {
		reverse_copy(itr, last, dst);
	}
	return res;
}


template<typename T, std::endian E, typename OI> requires (bufstream_write<T, OI>)
inline void pokeValue (const OI& itr, const T value) {
	check_endian_supported<E>();
	const u8* src = reinterpret_cast<const u8*>(&value);
	if constexpr (std::endian::native == E) {
		copy(src, src + sizeof(T), itr);
	} else {
		reverse_copy(src, src + sizeof(T), itr);
	}
}

#endif
