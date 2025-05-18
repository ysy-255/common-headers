#ifndef FILE_HPP
#define FILE_HPP

#include <cstdint>

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	inline constexpr bool SYSTEM_LITTLE_ENDIAN = true;
#else
	inline constexpr bool SYSTEM_LITTLE_ENDIAN = false;
#endif

inline std::vector<uint8_t> readFile(const std::string & path){
	std::ifstream file_ifstream(path, std::ios::binary | std::ios::ate);
	if(!file_ifstream.is_open()) return {};
	size_t file_size = file_ifstream.tellg();
	file_ifstream.seekg(0);
	std::vector<uint8_t> result(file_size);
	file_ifstream.read(reinterpret_cast<char*>(result.data()), file_size);
	file_ifstream.close();
	return result;
}

inline void writeFile(const std::string & path, const std::vector<uint8_t> & stream){
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
			result.push_back(f.path().string());
		}
	}
	return result;
}

template<typename T, typename Iter>
inline T readValue(Iter & itr, const bool is_little_endian){
	T res;
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(itr, itr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	else{
		std::reverse_copy(itr, itr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	itr += sizeof(T);
	return res;
}
template<typename T, typename Iter>
inline T readBE(Iter & itr){
	T res;
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		std::reverse_copy(itr, itr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	else{
		std::copy(itr, itr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	itr += sizeof(T);
	return res;
}
template<typename T, typename Iter>
inline T readLE(Iter & itr){
	T res;
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		std::copy(itr, itr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	else{
		std::reverse_copy(itr, itr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	itr += sizeof(T);
	return res;
}

template<typename T, typename Iter>
inline void writeValue(Iter & itr, const T value, const bool is_little_endian){
	const uint8_t* src = reinterpret_cast<const uint8_t*>(&value);
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(src, src + sizeof(T), itr);
	}
	else{
		std::reverse_copy(src, src + sizeof(T), itr);
	}
	itr += sizeof(T);
}
template<typename T, typename Iter>
inline void writeBE(Iter & itr, const T value){
	const uint8_t* src = reinterpret_cast<const uint8_t*>(&value);
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		std::reverse_copy(src, src + sizeof(T), itr);
	}
	else{
		std::copy(src, src + sizeof(T), itr);
	}
	itr += sizeof(T);
}
template<typename T, typename Iter>
inline void writeLE(Iter & itr, const T value){
	const uint8_t* src = reinterpret_cast<const uint8_t*>(&value);
	if constexpr (SYSTEM_LITTLE_ENDIAN){
		std::copy(src, src + sizeof(T), itr);
	}
	else{
		std::reverse_copy(src, src + sizeof(T), itr);
	}
	itr += sizeof(T);
}

template<typename Iter>
inline std::string readString(Iter & itr, const size_t size){
	std::string res(itr, itr + size);
	itr += size;
	return res;
}
template<typename Iter>
inline std::vector<uint8_t> readStream(Iter & itr, const size_t size){
	std::vector res(itr, itr + size);
	itr += size;
	return res;
}

#endif
