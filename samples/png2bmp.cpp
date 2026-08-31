#include <iostream>

#include "../bmp.hpp"
#include "../png.hpp"

using std::cerr;
using std::clog;
using std::endl;

int main (int argc, char * argv[]) {
	if (argc % 2 == 0) {
		cerr << "Usage: " << argv[0] << " <input.png> <output.bmp> [<input2.png> <output2.bmp> ...]" << endl;
		return 1;
	}
	for (int i = 1; i < argc; i += 2) {
		PNG3<RGBA<u16>> png;
		if (png.read(argv[i]) != PNG::Err::NONE) {
			cerr << "Error: Failed to read the PNG file: " << argv[i] << endl;
			return 1;
		}
		clog << "Read: " << argv[i] << endl;
		BMP bmp(png.ImageData());
		bmp.write(argv[i + 1]);
		clog << "Converted: " << argv[i] << " -> " << argv[i + 1] << endl;
	}
	return 0;
}
