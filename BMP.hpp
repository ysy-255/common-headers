#ifndef BMP_HPP
#define BMP_HPP

#include "./FILE.hpp"
#include "./IMAGE.hpp"

#define BMP_MINIMUM_SIZE 54

#define BMP_FILEHEADER_SIZE 14
#define BMP_INFOHEADER_SIZE 40

class BMP{
	public:

	BMP(){};

	BMP(const uint32_t W, const uint32_t H) :
		Width(W), Height(H), ImageData(W, H) {}

	BMP(const IMAGE_RGB<uint8_t> & img) :
		Width(img.width), Height(img.height), ImageData(img) {}

	BMP(const IMAGE_RGBA<uint8_t> & img) :
		Width(img.width), Height(img.height), ImageData(img) {}

	BMP(const BMP & bmp) :
		Width(bmp.Width), Height(bmp.Height), ImageData(bmp.ImageData) {}

	BMP(const std::string & path){
		read(path);
	}

	uint32_t Width;
	uint32_t Height;

	IMAGE_RGB<uint8_t> ImageData;

	inline std::vector<RGB<uint8_t>> & operator[](size_t h){
		return ImageData[h];
	}

	bool read(const std::string & path){
		BMPstream = readFile(path);
		if(BMPstream.size() < BMP_MINIMUM_SIZE) return false;
		const uint8_t* ptr = BMPstream.data();
		if(!read_FILEHEADER(ptr)) return false;
		if(!read_INFOHEADER(ptr)) return false;
		if(!read_BITMAP(ptr)) return false;
		return true;
	}

	bool write(const std::string & path){
		BMPstream.resize((Width * 3 + (Width & 0b11)) * Height + BMP_MINIMUM_SIZE);
		uint8_t* ptr = BMPstream.data();
		write_FILEHEADER(ptr);
		write_INFOHEADER(ptr);
		write_BITMAP(ptr);

		writeFile(path, BMPstream);
		return true;
	}

	protected:


	std::vector<uint8_t> BMPstream;

	static constexpr std::array<uint8_t, 2> correct_bfType = {'B', 'M'};

	bool read_FILEHEADER(const uint8_t* & ptr){
		if(!std::equal(ptr, ptr + 2, correct_bfType.data())) return false;
		ptr += 2;

		uint32_t bfSize = readData<uint32_t>(ptr, true);
		uint16_t bfReserved1 = readData<uint16_t>(ptr, true);
		uint16_t bfReserved2 = readData<uint16_t>(ptr, true);
		uint32_t bfOffBits = readData<uint32_t>(ptr, true);
		return true;
	}

	bool read_INFOHEADER(const uint8_t* & ptr){
		uint32_t biSize = readData<uint32_t>(ptr, true);
		if(biSize != BMP_INFOHEADER_SIZE) return false;
		Width = readData<uint32_t>(ptr, true);
		Height = readData<uint32_t>(ptr, true);
		uint16_t biPlanes = readData<uint16_t>(ptr, true);
		uint16_t biBitCount = readData<uint16_t>(ptr, true);
		uint32_t biCompression = readData<uint32_t>(ptr, true);
		uint32_t biSizeImage = readData<uint32_t>(ptr, true);
		uint32_t biXPelsPerMeter = readData<uint32_t>(ptr, true);
		uint32_t biYPelsPerMeter = readData<uint32_t>(ptr, true);
		uint32_t biClrUsed = readData<uint32_t>(ptr, true);
		uint32_t biClrImportant = readData<uint32_t>(ptr, true);

		if(biPlanes != 1) return false;
		if(biBitCount != 24) return false;
		if(biCompression != 0) return false;
		if(biClrUsed != 0) return false;
		return true;
	}

	bool read_BITMAP(const uint8_t* & ptr){
		ImageData = IMAGE_RGB<uint8_t>(Width, Height);
		uint8_t rest = Width & 0b11;
		for(uint32_t h = Height; h-- > 0;){
			for(uint32_t w = 0; w < Width; ++w){
				(*this)[h][w].B = *ptr++;
				(*this)[h][w].G = *ptr++;
				(*this)[h][w].R = *ptr++;
			}
			ptr += rest;
		}
		return true;
	}


	void write_FILEHEADER(uint8_t* & ptr){
		std::copy(correct_bfType.begin(), correct_bfType.end(), ptr);
		ptr += correct_bfType.size();
		writeData<uint32_t>(ptr, BMPstream.size(), true);
		writeData<uint16_t>(ptr, 0, true);
		writeData<uint16_t>(ptr, 0, true);
		writeData<uint32_t>(ptr, BMP_MINIMUM_SIZE, true);
		return;
	}

	void write_INFOHEADER(uint8_t* & ptr){
		writeData<uint32_t>(ptr, BMP_INFOHEADER_SIZE, true);
		writeData<uint32_t>(ptr, Width, true);
		writeData<uint32_t>(ptr, Height, true);
		writeData<uint16_t>(ptr, 1, true);
		writeData<uint16_t>(ptr, 24, true);
		writeData<uint32_t>(ptr, 0, true);
		writeData<uint32_t>(ptr, 0, true);
		writeData<uint32_t>(ptr, 0, true);
		writeData<uint32_t>(ptr, 0, true);
		writeData<uint32_t>(ptr, 0, true);
		writeData<uint32_t>(ptr, 0, true);
		return;
	}

	void write_BITMAP(uint8_t* & ptr){
		uint8_t rest = Width & 0b11;
		for(uint32_t h = Height; h-- > 0;){
			for(uint32_t w = 0; w < Width; ++w){
				*ptr++ = (*this)[h][w].B;
				*ptr++ = (*this)[h][w].G;
				*ptr++ = (*this)[h][w].R;
			}
			for(uint32_t rest_temp = rest; rest_temp-- > 0;) *ptr++ = 0;
		}
		return;
	}

};

#endif
