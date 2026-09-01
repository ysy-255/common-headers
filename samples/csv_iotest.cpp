#include <iostream>

#include "../file.hpp"
#include "../csv.hpp"

using std::cerr;
using std::clog;
using std::endl;

int main (int argc, char** argv) {
	if (argc % 2 == 0) {
		cerr << "Usage: " << argv[0] << " <input1.csv> <output1.csv> [<input2.csv> <output2.csv> ...]" << endl;
		return 1;
	}
	for (int i = 1; i < argc; i += 2) {
		CSV csv(argv[i]);
		if (csv.err != CSV::Err::NONE) {
			if (csv.err == CSV::Err::WARN) {
				clog << "Warning: CSV file read with warnings: " << argv[i] << endl;
			} else {
				cerr << "Error: Failed to read the CSV file: " << argv[i] << endl;
				return 1;
			}
		}
		clog << "Read: " << argv[i] << endl;
		csv.write(argv[i + 1]);
		clog << "Converted: " << argv[i] << " -> " << argv[i + 1] << endl;
	}
}
