#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include <math.h>
#include "tgaimage.h"

TGAImage::TGAImage() : data(nullptr), width(0), height(0), bytespp(0) {
}

TGAImage::TGAImage(int w, int h, int bpp) : data(nullptr), width(w), height(h), bytespp(bpp) {
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	memset(data, 0, nbytes);
}

TGAImage::TGAImage(const TGAImage& img) : data(nullptr), width(img.width), height(img.height), bytespp(img.bytespp) {
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	memcpy(data, img.data, nbytes);
}

TGAImage::~TGAImage() {
	if (data) {
		delete[] data;
	}
}

TGAImage& TGAImage::operator =(const TGAImage& img) {
	if (this != &img) {
		if (data) {
			delete[] data;
		}
		width = img.width;
		height = img.height;
		bytespp = img.bytespp;
		unsigned long nbytes = width * height * bytespp;
		data = new unsigned char[nbytes];
		memcpy(data, img.data, nbytes);
	}
	return *this;
}

// 从文件读取TGA图像数据
bool TGAImage::read_tga_file(const char* filename) {
	if (data) {
		delete[] data;
	}
	data = nullptr;
	std::ifstream in;
	in.open(filename, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		in.close();
		return false;
	}
	TGA_Header header;
	in.read((char*)&header, sizeof(header)); //读取文件头
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	//处理头部信息, 检查图像的宽高和每像素字节数等信息
	width = header.width;
	height = header.height;
	bytespp = header.bitsperpixel >> 3; //字节数是位数的8倍, 右移3位
	if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	unsigned long nbytes = bytespp * width * height; //图像数据大小
	data = new unsigned char[nbytes]; //根据图像数据大小初始化 指向图像数据的 字符数组指针data
	//根据不同的数据类型(如 未压缩的RGB 或 RLE压缩 的图像)读取数据
	if (3 == header.datatypecode || 2 == header.datatypecode) { //2代表数据类型为RGB/RGBA 非压缩数据, 3为灰度 非压缩数据
		in.read((char*)data, nbytes); //根据图像数据大小 将图像数据读入data
		if (!in.good()) { //如果流的状态不良，意味着出现了某种错误(如读取时发生错误或到达EOF)
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else if (10 == header.datatypecode || 11 == header.datatypecode) { //10代表数据类型为RGB/RGBA RLE, 11为灰度 RLE
		if (!load_rle_data(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else {
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}
	//根据图像描述符判断是否需要翻转图像
	if (!(header.imagedescriptor & 0x20)) {
		flip_vertically();
	}
	if (header.imagedescriptor & 0x10) {
		flip_horizontally();
	}
	std::cerr << width << "x" << height << "/" << bytespp * 8 << "\n"; //在标准错误流中输出信息, 如1920x1080/24
	in.close();
	return true;
}

// 从文件中读取RLE压缩格式的图像数据并解压到data数组中
bool TGAImage::load_rle_data(std::ifstream& in) {
	unsigned long pixelcount = width * height;
	unsigned long currentpixel = 0;
	unsigned long currentbyte = 0;
	TGAColor colorbuffer;
	do {
		unsigned char chunkheader = 0; //块头, 用来指明接下来的段是RLE还是RAW, 以及长度是多少
		chunkheader = in.get();
		if (!in.good()) {
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128) { //块头 < 128, 为RAW段, 块头值即为块内不重复的像素数量
			chunkheader++;
			for (int i = 0; i < chunkheader; i++) {
				in.read((char*)colorbuffer.bgra, bytespp); //读取一个像素的颜色值
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bytespp; t++) {
					data[currentbyte++] = colorbuffer.bgra[t]; //解码并存储像素数据
				}
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else {// 块头 >= 128, 为RLE段, 块头值-127即为块内重复的像素数量
			chunkheader -= 127;
			in.read((char*)colorbuffer.bgra, bytespp); //读取重复像素的颜色值
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++) {
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.bgra[t]; //解码并存储重复的像素数据
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	return true;
}

// 将图像数据保存为TGA文件, 支持选择是否使用RLE压缩
bool TGAImage::write_tga_file(const char* filename, bool rle) {
	unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char extension_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Header header;
	memset((void*)&header, 0, sizeof(header));
	header.bitsperpixel = bytespp << 3;
	header.width = width;
	header.height = height;
	header.datatypecode = (bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	header.imagedescriptor = 0x20; // top-left origin 顶部左起为原点
	out.write((char*)&header, sizeof(header));
	if (!out.good()) {
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) {
		out.write((char*)data, width * height * bytespp); //写入原始数据
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	}
	else {
		if (!unload_rle_data(out)) { //压缩并写入RLE数据
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write((char*)developer_area_ref, sizeof(developer_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)extension_area_ref, sizeof(extension_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)footer, sizeof(footer));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
// 将图像数据保存为RLE格式
bool TGAImage::unload_rle_data(std::ofstream& out) {
	const unsigned char max_chunk_length = 128; //最大块长度
	unsigned long npixels = width * height; //总像素数
	unsigned long curpix = 0; //当前像素
	while (curpix < npixels) {
		unsigned long chunkstart = curpix * bytespp;
		unsigned long curbyte = curpix * bytespp;
		unsigned char run_length = 1; //初始化运行长度为1, 用来控制chunk块的长度
		bool raw = true; //默认为原始数据
		//遍历完所有像素 或者 当前块长度达到最大值时, 跳出循环
		while (curpix + run_length < npixels && run_length < max_chunk_length) {
			bool succ_eq = true; //记录当前像素 和 紧邻的一个像素 是否相同
			for (int t = 0; succ_eq && t < bytespp; t++) {
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]); //比较连续两个像素是否相同
			}
			curbyte += bytespp;
			//运行长度为1时, 若succ_eq为true, 说明从curpix和curpix后面的像素相同, 存在相同像素, 需要压缩, 更新raw标记为false
			//若succ_eq为false, 说明从curpix和curpix后面的像素不同, 不需要压缩, 保持raw为true
			//通过这个判断, 用来划分当前的chunk块是用来记录 原始数据, 还是 需要压缩的数据
			if (1 == run_length) {
				raw = !succ_eq;
			}
			//若raw为true, 说明为原始数据, 即从curpix运行走过的像素都是不同的, 
			//而succ_eq为true, 说明遇到了相同像素, 此时跳出循环, 从curpix开始的run_length个数据都是不同的像素
			if (raw && succ_eq) {
				run_length--;
				break;
			}
			//若raw为false, 说明是需要压缩的数据, 即从curpix运行走过的像素都是相同的,
			//而succ_eq为false, 说明遇到了不同像素, 此时跳出循环, 从curpix开始的run_length个数据都是相同的像素
			if (!raw && !succ_eq) {
				break;
			}
			//raw为true, succ_eq为false, 说明从curpix运行走过的像素都是不同的
			//raw为false, succ_eq为true, 说明从curpix运行走过的像素都是相同的
			//继续往后运行
			run_length++;
		}
		curpix += run_length; //更新curpix到下一个要处理的像素
		//写入块头, 标明该块是原始数据还是重复数据, 小于128是原始数据, 大于等于128是重复数据
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		//写入数据块的像素信息, 如果是原始数据, 则写数据块的全部像素数据; 如果是重复数据, 则只写入一个像素
		out.write((char*)(data + chunkstart), (raw ? run_length * bytespp : bytespp));
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

// 获取指定位置的像素颜色
TGAColor TGAImage::get(int x, int y) {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return TGAColor();
	}
	return TGAColor(data + (x + y * width) * bytespp, bytespp);
}

// 设置指定位置的像素颜色
bool TGAImage::set(int x, int y, TGAColor& c) {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return false;
	}
	memcpy(data + (x + y * width) * bytespp, c.bgra, bytespp);
	return true;
}

bool TGAImage::set(int x, int y, const TGAColor& c) {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return false;
	}
	memcpy(data + (x + y * width) * bytespp, c.bgra, bytespp);
	return true;
}

int TGAImage::get_bytespp() {
	return bytespp;
}

int TGAImage::get_width() {
	return width;
}

int TGAImage::get_height() {
	return height;
}

// 水平翻转图像
bool TGAImage::flip_horizontally() {
	if (!data) return false;
	int half = width >> 1; //宽度的一半
	for (int i = 0; i < half; i++) {
		for (int j = 0; j < height; j++) {
			TGAColor c1 = get(i, j);
			TGAColor c2 = get(width - 1 - i, j);
			set(i, j, c2);
			set(width - 1 - i, j, c1);
		}
	}
	return true;
}

// 垂直翻转图像
bool TGAImage::flip_vertically() {
	if (!data) return false;
	unsigned long bytes_per_line = width * bytespp; //每行字节数
	unsigned char* line = new unsigned char[bytes_per_line]; //临时存储一行数据
	int half = height >> 1; //高度的一半
	for (int j = 0; j < half; j++) {
		unsigned long l1 = j * bytes_per_line; //当前行起始地址
		unsigned long l2 = (height - 1 - j) * bytes_per_line; //对应行起始地址
		memmove((void*)line, (void*)(data + l1), bytes_per_line); //复制上半部分行
		memmove((void*)(data + l1), (void*)(data + l2), bytes_per_line);
		memmove((void*)(data + l2), (void*)line, bytes_per_line);
	}
	delete[] line;
	return true;
}

// 返回图像数据的指针
unsigned char* TGAImage::buffer() {
	return data;
}

// 清空图像数据
void TGAImage::clear() {
	memset((void*)data, 0, width * height * bytespp);
}

// 缩放图像到新的宽度和高度(最近邻插值, 可考虑替换为双线性插值算法)
bool TGAImage::scale(int w, int h) {
	if (w <= 0 || h <= 0 || !data) return false;
	unsigned char* tdata = new unsigned char[w * h * bytespp]; //创建缩放后新图像的数据指针
	int nscanline = 0; //目标图像的扫描线偏移量
	int oscanline = 0; //源图像的扫描线偏移量
	int erry = 0; //竖直方向上的误差
	unsigned long nlinebytes = w * bytespp; //目标图像每一行像素占用的字节数
	unsigned long olinebytes = width * bytespp; //源图像每一行像素占用的字节数
	for (int j = 0; j < height; j++) {
		int errx = width - w; //水平方向上的误差, 用来计算ox是否需要向右移动
		int nx = -bytespp; //目标图像当前水平方向像素位置
		int ox = -bytespp; //源图像当前水平方向像素位置
		for (int i = 0; i < width; i++) {
			ox += bytespp; //水平方向上遍历源图像的像素
			errx += w; //水平方向误差增加
			while (errx >= (int)width) { //当误差大于源图像宽度时
				errx -= width; //误差回退
				nx += bytespp; //目标图像当前像素水平方向位置增加
				memcpy(tdata + nscanline + nx, data + oscanline + ox, bytespp); //将源像素复制到目标图像像素位置
			}
		}
		erry += h; //竖直方向误差增加
		oscanline += olinebytes; //更新源图像扫描线偏移量
		while (erry >= (int)height) { //当误差大于源图像高度时, 如果误差大于两倍的高度，表示需要跳过一行
			if (erry >= (int)height << 1) // it means we jump over a scanline
				memcpy(tdata + nscanline + nlinebytes, tdata + nscanline, nlinebytes);
			erry -= height; //误差回退
			nscanline += nlinebytes; //更新目标图像扫描线偏移量
		}
	}
	delete[] data;
	data = tdata;
	width = w;
	height = h;
	return true;
}