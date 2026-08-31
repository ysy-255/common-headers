#include "common-headers/png.hpp"

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
			printf("Failed to read %s\n", file_path.c_str());
		} else {
			printf("Successfully read %s\n", file_path.c_str());
			png.write(file_path.substr(0, file_path.size() - file_format.size()) + "_out.png");
		}
	}
}
