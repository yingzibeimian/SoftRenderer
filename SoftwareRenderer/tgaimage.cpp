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

// ���ļ���ȡTGAͼ������
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
	in.read((char*)&header, sizeof(header)); //��ȡ�ļ�ͷ
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	//����ͷ����Ϣ, ���ͼ��Ŀ�ߺ�ÿ�����ֽ�������Ϣ
	width = header.width;
	height = header.height;
	bytespp = header.bitsperpixel >> 3; //�ֽ�����λ����8��, ����3λ
	if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	unsigned long nbytes = bytespp * width * height; //ͼ�����ݴ�С
	data = new unsigned char[nbytes]; //����ͼ�����ݴ�С��ʼ�� ָ��ͼ�����ݵ� �ַ�����ָ��data
	//���ݲ�ͬ����������(�� δѹ����RGB �� RLEѹ�� ��ͼ��)��ȡ����
	if (3 == header.datatypecode || 2 == header.datatypecode) { //2������������ΪRGB/RGBA ��ѹ������, 3Ϊ�Ҷ� ��ѹ������
		in.read((char*)data, nbytes); //����ͼ�����ݴ�С ��ͼ�����ݶ���data
		if (!in.good()) { //�������״̬��������ζ�ų�����ĳ�ִ���(���ȡʱ��������򵽴�EOF)
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else if (10 == header.datatypecode || 11 == header.datatypecode) { //10������������ΪRGB/RGBA RLE, 11Ϊ�Ҷ� RLE
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
	//����ͼ���������ж��Ƿ���Ҫ��תͼ��
	if (!(header.imagedescriptor & 0x20)) {
		flip_vertically();
	}
	if (header.imagedescriptor & 0x10) {
		flip_horizontally();
	}
	std::cerr << width << "x" << height << "/" << bytespp * 8 << "\n"; //�ڱ�׼�������������Ϣ, ��1920x1080/24
	in.close();
	return true;
}

// ���ļ��ж�ȡRLEѹ����ʽ��ͼ�����ݲ���ѹ��data������
bool TGAImage::load_rle_data(std::ifstream& in) {
	unsigned long pixelcount = width * height;
	unsigned long currentpixel = 0;
	unsigned long currentbyte = 0;
	TGAColor colorbuffer;
	do {
		unsigned char chunkheader = 0; //��ͷ, ����ָ���������Ķ���RLE����RAW, �Լ������Ƕ���
		chunkheader = in.get();
		if (!in.good()) {
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128) { //��ͷ < 128, ΪRAW��, ��ͷֵ��Ϊ���ڲ��ظ�����������
			chunkheader++;
			for (int i = 0; i < chunkheader; i++) {
				in.read((char*)colorbuffer.bgra, bytespp); //��ȡһ�����ص���ɫֵ
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bytespp; t++) {
					data[currentbyte++] = colorbuffer.bgra[t]; //���벢�洢��������
				}
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else {// ��ͷ >= 128, ΪRLE��, ��ͷֵ-127��Ϊ�����ظ�����������
			chunkheader -= 127;
			in.read((char*)colorbuffer.bgra, bytespp); //��ȡ�ظ����ص���ɫֵ
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++) {
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.bgra[t]; //���벢�洢�ظ�����������
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

// ��ͼ�����ݱ���ΪTGA�ļ�, ֧��ѡ���Ƿ�ʹ��RLEѹ��
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
	header.imagedescriptor = 0x20; // top-left origin ��������Ϊԭ��
	out.write((char*)&header, sizeof(header));
	if (!out.good()) {
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) {
		out.write((char*)data, width * height * bytespp); //д��ԭʼ����
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	}
	else {
		if (!unload_rle_data(out)) { //ѹ����д��RLE����
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
// ��ͼ�����ݱ���ΪRLE��ʽ
bool TGAImage::unload_rle_data(std::ofstream& out) {
	const unsigned char max_chunk_length = 128; //���鳤��
	unsigned long npixels = width * height; //��������
	unsigned long curpix = 0; //��ǰ����
	while (curpix < npixels) {
		unsigned long chunkstart = curpix * bytespp;
		unsigned long curbyte = curpix * bytespp;
		unsigned char run_length = 1; //��ʼ�����г���Ϊ1, ��������chunk��ĳ���
		bool raw = true; //Ĭ��Ϊԭʼ����
		//�������������� ���� ��ǰ�鳤�ȴﵽ���ֵʱ, ����ѭ��
		while (curpix + run_length < npixels && run_length < max_chunk_length) {
			bool succ_eq = true; //��¼��ǰ���� �� ���ڵ�һ������ �Ƿ���ͬ
			for (int t = 0; succ_eq && t < bytespp; t++) {
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]); //�Ƚ��������������Ƿ���ͬ
			}
			curbyte += bytespp;
			//���г���Ϊ1ʱ, ��succ_eqΪtrue, ˵����curpix��curpix�����������ͬ, ������ͬ����, ��Ҫѹ��, ����raw���Ϊfalse
			//��succ_eqΪfalse, ˵����curpix��curpix��������ز�ͬ, ����Ҫѹ��, ����rawΪtrue
			//ͨ������ж�, �������ֵ�ǰ��chunk����������¼ ԭʼ����, ���� ��Ҫѹ��������
			if (1 == run_length) {
				raw = !succ_eq;
			}
			//��rawΪtrue, ˵��Ϊԭʼ����, ����curpix�����߹������ض��ǲ�ͬ��, 
			//��succ_eqΪtrue, ˵����������ͬ����, ��ʱ����ѭ��, ��curpix��ʼ��run_length�����ݶ��ǲ�ͬ������
			if (raw && succ_eq) {
				run_length--;
				break;
			}
			//��rawΪfalse, ˵������Ҫѹ��������, ����curpix�����߹������ض�����ͬ��,
			//��succ_eqΪfalse, ˵�������˲�ͬ����, ��ʱ����ѭ��, ��curpix��ʼ��run_length�����ݶ�����ͬ������
			if (!raw && !succ_eq) {
				break;
			}
			//rawΪtrue, succ_eqΪfalse, ˵����curpix�����߹������ض��ǲ�ͬ��
			//rawΪfalse, succ_eqΪtrue, ˵����curpix�����߹������ض�����ͬ��
			//������������
			run_length++;
		}
		curpix += run_length; //����curpix����һ��Ҫ���������
		//д���ͷ, �����ÿ���ԭʼ���ݻ����ظ�����, С��128��ԭʼ����, ���ڵ���128���ظ�����
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		//д�����ݿ��������Ϣ, �����ԭʼ����, ��д���ݿ��ȫ����������; ������ظ�����, ��ֻд��һ������
		out.write((char*)(data + chunkstart), (raw ? run_length * bytespp : bytespp));
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

// ��ȡָ��λ�õ�������ɫ
TGAColor TGAImage::get(int x, int y) {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return TGAColor();
	}
	return TGAColor(data + (x + y * width) * bytespp, bytespp);
}

// ����ָ��λ�õ�������ɫ
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

// ˮƽ��תͼ��
bool TGAImage::flip_horizontally() {
	if (!data) return false;
	int half = width >> 1; //��ȵ�һ��
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

// ��ֱ��תͼ��
bool TGAImage::flip_vertically() {
	if (!data) return false;
	unsigned long bytes_per_line = width * bytespp; //ÿ���ֽ���
	unsigned char* line = new unsigned char[bytes_per_line]; //��ʱ�洢һ������
	int half = height >> 1; //�߶ȵ�һ��
	for (int j = 0; j < half; j++) {
		unsigned long l1 = j * bytes_per_line; //��ǰ����ʼ��ַ
		unsigned long l2 = (height - 1 - j) * bytes_per_line; //��Ӧ����ʼ��ַ
		memmove((void*)line, (void*)(data + l1), bytes_per_line); //�����ϰ벿����
		memmove((void*)(data + l1), (void*)(data + l2), bytes_per_line);
		memmove((void*)(data + l2), (void*)line, bytes_per_line);
	}
	delete[] line;
	return true;
}

// ����ͼ�����ݵ�ָ��
unsigned char* TGAImage::buffer() {
	return data;
}

// ���ͼ������
void TGAImage::clear() {
	memset((void*)data, 0, width * height * bytespp);
}

// ����ͼ���µĿ�Ⱥ͸߶�(����ڲ�ֵ, �ɿ����滻Ϊ˫���Բ�ֵ�㷨)
bool TGAImage::scale(int w, int h) {
	if (w <= 0 || h <= 0 || !data) return false;
	unsigned char* tdata = new unsigned char[w * h * bytespp]; //�������ź���ͼ�������ָ��
	int nscanline = 0; //Ŀ��ͼ���ɨ����ƫ����
	int oscanline = 0; //Դͼ���ɨ����ƫ����
	int erry = 0; //��ֱ�����ϵ����
	unsigned long nlinebytes = w * bytespp; //Ŀ��ͼ��ÿһ������ռ�õ��ֽ���
	unsigned long olinebytes = width * bytespp; //Դͼ��ÿһ������ռ�õ��ֽ���
	for (int j = 0; j < height; j++) {
		int errx = width - w; //ˮƽ�����ϵ����, ��������ox�Ƿ���Ҫ�����ƶ�
		int nx = -bytespp; //Ŀ��ͼ��ǰˮƽ��������λ��
		int ox = -bytespp; //Դͼ��ǰˮƽ��������λ��
		for (int i = 0; i < width; i++) {
			ox += bytespp; //ˮƽ�����ϱ���Դͼ�������
			errx += w; //ˮƽ�����������
			while (errx >= (int)width) { //��������Դͼ����ʱ
				errx -= width; //������
				nx += bytespp; //Ŀ��ͼ��ǰ����ˮƽ����λ������
				memcpy(tdata + nscanline + nx, data + oscanline + ox, bytespp); //��Դ���ظ��Ƶ�Ŀ��ͼ������λ��
			}
		}
		erry += h; //��ֱ�����������
		oscanline += olinebytes; //����Դͼ��ɨ����ƫ����
		while (erry >= (int)height) { //��������Դͼ��߶�ʱ, ��������������ĸ߶ȣ���ʾ��Ҫ����һ��
			if (erry >= (int)height << 1) // it means we jump over a scanline
				memcpy(tdata + nscanline + nlinebytes, tdata + nscanline, nlinebytes);
			erry -= height; //������
			nscanline += nlinebytes; //����Ŀ��ͼ��ɨ����ƫ����
		}
	}
	delete[] data;
	data = tdata;
	width = w;
	height = h;
	return true;
}