#include <iostream>

#include "../png.hpp"

using std::cerr;
using std::clog;
using std::endl;

int main (int argc, char** argv) {
	if (argc % 2 == 0) {
		cerr << "Usage: " << argv[0] << " <input1.png> <output1.png> [<input2.png> <output2.png> ...]" << endl;
		return 1;
	}
	for (int i = 1; i < argc; i += 2) {
		using PNG_t = PNG3<RGBA<u16>>;
		PNG_t img;
		if (img.read(argv[i]) != PNG_t::Err::NONE) {
			cerr << "Error: Failed to read the PNG file: " << argv[i] << endl;
			return 1;
		}
		clog << "Read: " << argv[i] << endl;
		img.write(argv[i + 1], 9);
		clog << "Converted: " << argv[i] << " -> " << argv[i + 1] << endl;
	}
}
