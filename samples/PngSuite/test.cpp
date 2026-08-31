#include <iostream>

#include "common-headers/png.hpp"

using std::clog;
using std::cerr;
using std::endl;

const string base_path = "samples/PngSuite/files/";
const string file_format = ".png";
const string tail_filename = "_out";
const string tail_name = tail_filename + file_format;

int main() {
	auto filelist = getFileList(base_path);
	sort(filelist.begin(), filelist.end());
	for (const auto& filename : filelist) {
		if (filename.size() > tail_name.size()) {
			if (filename.substr(filename.size() - tail_name.size()) == tail_name) {
				continue;
			}
		}
		if (filename.size() < file_format.size()) continue;
		if (filename.substr(filename.size() - file_format.size()) != file_format) continue;
		const auto file_path = base_path + filename;
		PNG png;
		if (png.read(file_path) != PNG::Err::NONE) {
			cerr << "Failed to read " << file_path << endl;
		} else {
			clog << "Successfully read " << file_path << endl;
			png.write(file_path.substr(0, file_path.size() - file_format.size()) + "_out.png");
		}
	}
}
