#ifndef FILE_HPP
#define FILE_HPP

#include <cstdint>

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	const bool SYSTEM_LITTLE_ENDIAN = true;
#else
	const bool SYSTEM_LITTLE_ENDIAN = false;
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

template<typename T>
inline T readData(const uint8_t* & ptr, const bool is_little_endian){
	T res;
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(ptr, ptr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	else{
		std::reverse_copy(ptr, ptr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	ptr += sizeof(T);
	return res;
}
template<typename T>
inline T readData(std::vector<uint8_t>::const_iterator & ptr, const bool is_little_endian){
	T res;
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(ptr, ptr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	else{
		std::reverse_copy(ptr, ptr + sizeof(T), reinterpret_cast<uint8_t*>(&res));
	}
	ptr += sizeof(T);
	return res;
}

template<typename T>
inline void writeData(uint8_t* & ptr, const T value, const bool is_little_endian){
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(T), ptr);
	}
	else{
		std::reverse_copy(reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(T), ptr);
	}
	ptr += sizeof(T);
	return;
}
template<typename T>
inline void writeData(std::vector<uint8_t>::const_iterator & ptr, const T value, const bool is_little_endian){
	if(is_little_endian == SYSTEM_LITTLE_ENDIAN){
		std::copy(reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(T), ptr);
	}
	else{
		std::reverse_copy(reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(T), ptr);
	}
	ptr += sizeof(T);
	return;
}

#endif
