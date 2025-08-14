#include <iostream>

#include "../file.hpp"
#include "../csv.hpp"

const std::string src_path = "cases/csv/in/";
const std::string dst_path = "cases/csv/out/";

int main(){
	auto paths = getFileList(src_path);
	for(auto path : paths){
		std::clog << path << ":\n";
		CSV csv(src_path + path);
		for(const auto & row : csv){
			for(const std::string & el : row){
				std::clog << el << ',';
			}
			std::clog << '\n';
		}
		std::clog << '\n';
		csv.write(dst_path + path);
	}
}
