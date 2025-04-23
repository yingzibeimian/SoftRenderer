#ifndef __IMAGE_H__ //查看此头文件是否已经被包含
#define __IMAGE_H__ //如果没有, 定义它

#include <fstream>

#pragma pack(push,1) //告诉编译器使用 1 字节对齐，确保结构体内存布局紧凑，不出现填充字节
//TGA_Header是TGA(Targa)图像文件格式的头部信息, 包含了图像的基本属性
struct TGA_Header {
	char idlength; //图像文件中附加的标识符(id)长度
	char colormaptype; //调色板类型(0表示没有调色板, 1表示有调色板)
	char datatypecode; //数据类型
	short colormaporigin; //调色板相关信息
	short colormaplength;
	char colormapdepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char  bitsperpixel; //每个像素的位数(如24表示RGB, 每个像素3字节)
	char  imagedescriptor; //图像描述符, 通常用于表示图像的方向等信息
};
#pragma pack(pop)


//TGAColor代表TGA图像中的一个像素的颜色
struct TGAColor {
	union {
		struct {
			unsigned char b, g, r, a; //blue green red alpha(透明度)通道
		};
		unsigned char bgra[4]; //可以同时表示 b g r a 4个通道的数据
		unsigned int val; //四个颜色通道的合并值
	};
	int bytespp; //bytes per pixel每个像素的字节数

	TGAColor() : val(0), bytespp(1) {
		for (int i = 0; i < 4; i++) {
			bgra[i] = 0;
		}
	}

	TGAColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A = 255) : bgra(), bytespp(4) {
		bgra[0] = B;
		bgra[1] = G;
		bgra[2] = R;
		bgra[3] = A;
	}

	TGAColor(unsigned char v) : bgra(), bytespp(1) { //存储灰度图像素颜色值
		bgra[0] = v;
		for (int i = 1; i < 4; i++) {
			bgra[i] = 0;
		}
	}

	TGAColor(const unsigned char* p, unsigned char bpp) : bgra(), bytespp(bpp) {
		for (int i = 0; i < (int)bpp; i++) {
			bgra[i] = p[i]; //将字节数组p中的颜色值填充到bgra数组
		}
		for (int i = bpp; i < 4; i++) {
			bgra[i] = 0; //如果是灰度图, 则除bgra[0]以外用0填充
		}
	}

	TGAColor& operator =(const TGAColor& c) { //重载赋值运算符
		if (this != &c) {
			bytespp = c.bytespp;
			val = c.val;
		}
		return *this;
	}

	TGAColor operator *(float intensity) const {
		TGAColor res = *this;
		intensity = (intensity > 1.f ? 1.f : (intensity < 0.f ? 0.f : intensity));
		for (int i = 0; i < 4; i++) {
			res.bgra[i] = bgra[i] * intensity;
		}
		return res;
	}

	unsigned char& operator[](const int i) {
		return bgra[i];
	}
};

//TGAImage表示一幅TGA图像, 包含图像的基本信息和对图像进行操作的功能
class TGAImage {
protected:
	unsigned char* data; //指向图像数据的指针
	int width; //图像宽度
	int height; //图像高度
	int bytespp; //每个像素的字节数

	bool   load_rle_data(std::ifstream& in); //加载压缩的RLE(Run-Length Encoding)数据
	bool unload_rle_data(std::ofstream& out); //将图像数据保存为RLE格式
public:
	enum Format {
		GRAYSCALE = 1, RGB = 3, RGBA = 4 //图像的三种格式, 对应的每个像素字节数为 灰度(1) RGB(3) RGBA(4)
	};

	TGAImage();
	TGAImage(int w, int h, int bpp);
	TGAImage(const TGAImage& img);
	~TGAImage();

	bool read_tga_file(const char* filename); //从文件读取TGA图像数据
	bool write_tga_file(const char* filename, bool rle = true); //将图像数据保存为TGA文件, 支持选择是否使用RLE压缩
	bool flip_horizontally(); //水平翻转图像
	bool flip_vertically(); //垂直翻转图像
	bool scale(int w, int h); //缩放图像到新的宽度和高度
	TGAColor get(int x, int y); //获取指定位置的像素颜色
	bool set(int x, int y, TGAColor& c); //设置指定位置的像素颜色
	bool set(int x, int y, const TGAColor& c);
	
	TGAImage& operator =(const TGAImage& img); //重载赋值运算符
	int get_width();
	int get_height();
	int get_bytespp();
	unsigned char* buffer(); //返回图像数据的指针
	void clear(); //清空图像数据
};

#endif //__IMAGE_H__