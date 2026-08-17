#ifndef FILE_HPP
#define FILE_HPP

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

#include "int.hpp"

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::copy;
using std::reverse_copy;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	inline constexpr bool SYSTEM_LITTLE_ENDIAN = true;
#else
	inline constexpr bool SYSTEM_LITTLE_ENDIAN = false;
#endif

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

template<typename T, typename II>
inline T readValue (II& itr, const bool is_little_endian) {
	T res;
	II last = itr + sizeof(T);
	if (is_little_endian == SYSTEM_LITTLE_ENDIAN) {
		copy(itr, last, reinterpret_cast<u8*>(&res));
	} else {
		reverse_copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	itr = last;
	return res;
}
template<typename T, typename II>
inline T readBE (II& itr) {
	T res;
	II last = itr + sizeof(T);
	if constexpr (SYSTEM_LITTLE_ENDIAN) {
		reverse_copy(itr, last, reinterpret_cast<u8*>(&res));
	} else {
		copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	itr = last;
	return res;
}
template<typename T, typename II>
inline T readLE (II& itr) {
	T res;
	II last = itr + sizeof(T);
	if constexpr (SYSTEM_LITTLE_ENDIAN) {
		copy(itr, last, reinterpret_cast<u8*>(&res));
	} else {
		reverse_copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	itr = last;
	return res;
}

template<typename T, typename OI>
inline void writeValue (OI& itr, const T value, const bool is_little_endian) {
	const u8* src = reinterpret_cast<const u8*>(&value);
	if (is_little_endian == SYSTEM_LITTLE_ENDIAN) {
		itr = copy(src, src + sizeof(T), itr);
	} else {
		itr = reverse_copy(src, src + sizeof(T), itr);
	}
}
template<typename T, typename OI>
inline void writeBE (OI& itr, const T value) {
	const u8* src = reinterpret_cast<const u8*>(&value);
	if constexpr (SYSTEM_LITTLE_ENDIAN) {
		itr = reverse_copy(src, src + sizeof(T), itr);
	} else {
		itr = copy(src, src + sizeof(T), itr);
	}
}
template<typename T, typename OI>
inline void writeLE (OI& itr, const T value) {
	const u8* src = reinterpret_cast<const u8*>(&value);
	if constexpr (SYSTEM_LITTLE_ENDIAN) {
		itr = copy(src, src + sizeof(T), itr);
	} else {
		itr = reverse_copy(src, src + sizeof(T), itr);
	}
}

template<typename II>
inline std::string readString (II& itr, const size_t size) {
	std::string res(itr, itr + size);
	itr += size;
	return res;
}
template<typename II>
inline vector<u8> readBytes (II& itr, const size_t size) {
	vector<u8> res(itr, itr + size);
	itr += size;
	return res;
}

template<typename OI>
inline void writeString (OI& itr, const string& str) {
	itr = std::copy(str.begin(), str.end(), itr);
}
template<typename OI>
inline void writeBytes (OI& itr, const vector<u8>& bytes) {
	itr = std::copy(bytes.begin(), bytes.end(), itr);
}

#endif
