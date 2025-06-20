#ifndef BMP_HPP
#define BMP_HPP

#include "./file.hpp"
#include "./image.hpp"

#define BMP_MINIMUM_SIZE 54

#define BMP_FILEHEADER_SIZE 14
#define BMP_INFOHEADER_SIZE 40

class BMP{
public:

	enum class Err{
		NONE, // 正常に処理されたはずです
		UNKNOWN_TYPE, // ファイルが BM から始まりません
		UNRECOGNIZABLE, // BMPファイルとして認識できません
		UNSUPPORTED_INFOHEADER, // サポートしていないINFOHEADERです
		UNSUPPORTED_BitCount, // サポートしていないビット数です
		UNSUPPORTED_Compression, // サポートしていない圧縮形式です
		UNSUPPORTED // その他のサポートしていない要素があります
	};

	BMP() = default;
	BMP(u32 Height, u32 Width)   : H(Height), W(Width), data(H, W)     {}
	BMP(const Image_RGB8 & img)  : H(img.H),  W(img.W), data(img)      {}
	BMP(const Image_RGBA8 & img) : H(img.H),  W(img.W), data(img)      {}
	BMP(const BMP & bmp)         : H(bmp.H),  W(bmp.W), data(bmp.data) {}

	BMP & operator=(const BMP & other);

	const Image_RGB8 & ImageData() const{ return data; }
	std::vector<RGB8> & operator[](u32 h){ return data[h]; }

	BMP(const std::string & path){ read(path); }

	Err read(const std::string & path);
	void write(const std::string & path);

	u32 H, W;


protected:

	Image_RGB8 data;

	std::vector<u8> BMPstream;

	Err read_FILEHEADER(std::vector<u8>::const_iterator &);
	Err read_INFOHEADER(std::vector<u8>::const_iterator &);
	Err read_BITMAP(std::vector<u8>::const_iterator &);

	void write_FILEHEADER(std::vector<u8>::iterator &);
	void write_INFOHEADER(std::vector<u8>::iterator &);
	void write_BITMAP(std::vector<u8>::iterator &);

};

BMP & BMP::operator=(const BMP & other){
	if(this != &other){
		H = other.H;
		W = other.W;
		data = other.data;
	}
	return *this;
}

BMP::Err BMP::read(const std::string & path){
	BMPstream = readFile(path);
	if(BMPstream.size() < BMP_MINIMUM_SIZE) return Err::UNRECOGNIZABLE;
	std::vector<u8>::const_iterator itr = BMPstream.begin();
	Err e = Err::NONE;
	if((e = read_FILEHEADER(itr)) != Err::NONE) return e;
	if((e = read_INFOHEADER(itr)) != Err::NONE) return e;
	if((e = read_BITMAP(itr)) != Err::NONE) return e;
	return e;
}

BMP::Err BMP::read_FILEHEADER(std::vector<u8>::const_iterator & itr){
	if(std::string(itr, itr + 2) != "BM") return Err::UNKNOWN_TYPE;
	itr += 2;
	u32 bfSize = readLE<u32>(itr);
	[[maybe_unused]] u16 bfReserved1 = readLE<u16>(itr);
	[[maybe_unused]] u16 bfReserved2 = readLE<u16>(itr);
	u32 bfOffBits = readLE<u32>(itr);
	return Err::NONE;
}

BMP::Err BMP::read_INFOHEADER(std::vector<u8>::const_iterator & itr){
	u32 biSize = readLE<u32>(itr);
	if(biSize != BMP_INFOHEADER_SIZE) return Err::UNSUPPORTED_INFOHEADER;
	W = readLE<u32>(itr);
	H = readLE<u32>(itr);
	u16 biPlanes = readLE<u16>(itr);
	u16 biBitCount = readLE<u16>(itr);
	u32 biCompression = readLE<u32>(itr);
	u32 biSizeImage = readLE<u32>(itr);
	u32 biXPelsPerMeter = readLE<u32>(itr);
	u32 biYPelsPerMeter = readLE<u32>(itr);
	u32 biClrUsed = readLE<u32>(itr);
	u32 biClrImportant = readLE<u32>(itr);
	if(biPlanes != 1) return Err::UNSUPPORTED;
	if(biBitCount != 24) return Err::UNSUPPORTED_BitCount;
	if(biCompression != 0) return Err::UNSUPPORTED_Compression;
	if(biClrUsed != 0) return Err::UNSUPPORTED;
	return Err::NONE;
}

BMP::Err BMP::read_BITMAP(std::vector<u8>::const_iterator & itr){
	data = Image_RGB8(H, W);
	u8 rest = W & 0b11;
	if((W * 3 + rest) * H > BMPstream.end() - itr) return Err::UNRECOGNIZABLE;
	for(u32 h = H; h-- > 0;){
		for(u32 w = 0; w < W; ++w){
			data[h][w].B = *itr++;
			data[h][w].G = *itr++;
			data[h][w].R = *itr++;
		}
		itr += rest;
	}
	return Err::NONE;
}

void BMP::write(const std::string & path){
	BMPstream.resize((W * 3 + (W & 0b11)) * H + BMP_MINIMUM_SIZE);
	std::vector<u8>::iterator itr = BMPstream.begin();
	write_FILEHEADER(itr);
	write_INFOHEADER(itr);
	write_BITMAP(itr);
	writeFile(path, BMPstream);
}

void BMP::write_FILEHEADER(std::vector<u8>::iterator & itr){
	writeString(itr, "BM");
	writeLE<u32>(itr, BMPstream.size());
	writeLE<u16>(itr, 0);
	writeLE<u16>(itr, 0);
	writeLE<u32>(itr, BMP_MINIMUM_SIZE);
}

void BMP::write_INFOHEADER(std::vector<u8>::iterator & itr){
	writeLE<u32>(itr, BMP_INFOHEADER_SIZE);
	writeLE<u32>(itr, W);
	writeLE<u32>(itr, H);
	writeLE<u16>(itr, 1);
	writeLE<u16>(itr, 24);
	writeLE<u32>(itr, 0);
	writeLE<u32>(itr, 0);
	writeLE<u32>(itr, 0);
	writeLE<u32>(itr, 0);
	writeLE<u32>(itr, 0);
	writeLE<u32>(itr, 0);
}

void BMP::write_BITMAP(std::vector<u8>::iterator & itr){
	u8 rest = W & 0b11;
	for(u32 h = H; h-- > 0;){
		for(u32 w = 0; w < W; ++w){
			*itr++ = data[h][w].B;
			*itr++ = data[h][w].G;
			*itr++ = data[h][w].R;
		}
		itr = std::fill_n(itr, rest, 0);
	}
}

#endif
