#include <iostream>

#include "common-headers/file.hpp"
#include "common-headers/csv.hpp"

// https://github.com/max-mapper/csv-spectrum
const string base_path = "samples/csv-spectrum/files/";
const string file_format = ".csv";
const string tail_filename = "_out";
const string tail_name = tail_filename + file_format;

using std::clog;
using std::cerr;
using std::endl;
using std::sort;

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
		CSV csv;
		if (csv.read(file_path) != CSV::Err::NONE) {
			if (csv.err == CSV::Err::WARN) {
				cerr << "Warning reading CSV file: " << file_path << endl;
				switch (csv.warn) {
					case CSV::Warn::UNEXPECT_AFTER_DQUOTE:
						cerr << "Unexpected character after closing double quote." << endl;
						break;
					case CSV::Warn::UNCLOSED_DQUOTE:
						cerr << "Unclosed double quote." << endl;
						break;
					default: break;
				}
			} else {
				cerr << "Error reading CSV file: " << file_path << endl;
			}
		} else {
			clog << "Successfully read " << file_path << endl;
			csv.write(file_path.substr(0, file_path.size() - file_format.size()) + tail_name);
		}
	}
}
