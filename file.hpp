#ifndef FILE_HPP
#define FILE_HPP

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

#include "int.hpp"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	inline constexpr bool SYSTEM_LITTLE_ENDIAN = true;
#else
	inline constexpr bool SYSTEM_LITTLE_ENDIAN = false;
#endif

inline std::vector<u8> readFile(const std::string & path){
	std::ifstream file_ifstream(path, std::ios::binary | std::ios::ate);
	if(!file_ifstream.is_open()) return {};
	size_t file_size = file_ifstream.tellg();
	file_ifstream.seekg(0);
	std::vector<u8> result(file_size);
	file_ifstream.read(reinterpret_cast<char*>(result.data()), file_size);
	file_ifstream.close();
	return result;
}

inline void writeFile(const std::string & path, const std::vector<u8> & stream){
	std::ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char*>(stream.data()), stream.size());
	out.close();
	return;
}


inline std::vector<std::string> getFileList(const std::string & folder_path){
	std::vector<std::string> result;
	auto folder_files = std::filesystem::directory_iterator(folder_path);
	for(auto & f : folder_files){
		if(f.is_regular_file()){
			result.push_back(f.path().filename().string());
		}
	}
	return result;
}

template<typename T, typename II>
inline T readValue(II & itr, const bool is_little_endian){
	T res;
	II last = itr + sizeof(T);
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	else{
		std::reverse_copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	itr = last;
	return res;
}
template<typename T, typename II>
inline T readBE(II & itr){
	T res;
	II last = itr + sizeof(T);
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		std::reverse_copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	else{
		std::copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	itr = last;
	return res;
}
template<typename T, typename II>
inline T readLE(II & itr){
	T res;
	II last = itr + sizeof(T);
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		std::copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	else{
		std::reverse_copy(itr, last, reinterpret_cast<u8*>(&res));
	}
	itr = last;
	return res;
}

template<typename T, typename OI>
inline void writeValue(OI & itr, const T value, const bool is_little_endian){
	const u8* src = reinterpret_cast<const u8*>(&value);
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		itr = std::copy(src, src + sizeof(T), itr);
	}
	else{
		itr = std::reverse_copy(src, src + sizeof(T), itr);
	}
}
template<typename T, typename OI>
inline void writeBE(OI & itr, const T value){
	const u8* src = reinterpret_cast<const u8*>(&value);
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		itr = std::reverse_copy(src, src + sizeof(T), itr);
	}
	else{
		itr = std::copy(src, src + sizeof(T), itr);
	}
}
template<typename T, typename OI>
inline void writeLE(OI & itr, const T value){
	const u8* src = reinterpret_cast<const u8*>(&value);
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		itr = std::copy(src, src + sizeof(T), itr);
	}
	else{
		itr = std::reverse_copy(src, src + sizeof(T), itr);
	}
}

template<typename II>
inline std::string readString(II & itr, const size_t size){
	std::string res(itr, itr + size);
	itr += size;
	return res;
}
template<typename II>
inline std::vector<u8> readBytes(II & itr, const size_t size){
	std::vector<u8> res(itr, itr + size);
	itr += size;
	return res;
}

template<typename OI>
inline void writeString(OI & itr, const std::string & str){
	itr = std::copy(str.begin(), str.end(), itr);
}
template<typename OI>
inline void writeBytes(OI & itr, const std::vector<u8> & bytes){
	itr = std::copy(bytes.begin(), bytes.end(), itr);
}

#endif
