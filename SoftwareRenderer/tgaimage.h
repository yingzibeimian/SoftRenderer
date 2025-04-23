#ifndef __IMAGE_H__ //�鿴��ͷ�ļ��Ƿ��Ѿ�������
#define __IMAGE_H__ //���û��, ������

#include <fstream>

#pragma pack(push,1) //���߱�����ʹ�� 1 �ֽڶ��룬ȷ���ṹ���ڴ沼�ֽ��գ�����������ֽ�
//TGA_Header��TGA(Targa)ͼ���ļ���ʽ��ͷ����Ϣ, ������ͼ��Ļ�������
struct TGA_Header {
	char idlength; //ͼ���ļ��и��ӵı�ʶ��(id)����
	char colormaptype; //��ɫ������(0��ʾû�е�ɫ��, 1��ʾ�е�ɫ��)
	char datatypecode; //��������
	short colormaporigin; //��ɫ�������Ϣ
	short colormaplength;
	char colormapdepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char  bitsperpixel; //ÿ�����ص�λ��(��24��ʾRGB, ÿ������3�ֽ�)
	char  imagedescriptor; //ͼ��������, ͨ�����ڱ�ʾͼ��ķ������Ϣ
};
#pragma pack(pop)


//TGAColor����TGAͼ���е�һ�����ص���ɫ
struct TGAColor {
	union {
		struct {
			unsigned char b, g, r, a; //blue green red alpha(͸����)ͨ��
		};
		unsigned char bgra[4]; //����ͬʱ��ʾ b g r a 4��ͨ��������
		unsigned int val; //�ĸ���ɫͨ���ĺϲ�ֵ
	};
	int bytespp; //bytes per pixelÿ�����ص��ֽ���

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

	TGAColor(unsigned char v) : bgra(), bytespp(1) { //�洢�Ҷ�ͼ������ɫֵ
		bgra[0] = v;
		for (int i = 1; i < 4; i++) {
			bgra[i] = 0;
		}
	}

	TGAColor(const unsigned char* p, unsigned char bpp) : bgra(), bytespp(bpp) {
		for (int i = 0; i < (int)bpp; i++) {
			bgra[i] = p[i]; //���ֽ�����p�е���ɫֵ��䵽bgra����
		}
		for (int i = bpp; i < 4; i++) {
			bgra[i] = 0; //����ǻҶ�ͼ, ���bgra[0]������0���
		}
	}

	TGAColor& operator =(const TGAColor& c) { //���ظ�ֵ�����
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

//TGAImage��ʾһ��TGAͼ��, ����ͼ��Ļ�����Ϣ�Ͷ�ͼ����в����Ĺ���
class TGAImage {
protected:
	unsigned char* data; //ָ��ͼ�����ݵ�ָ��
	int width; //ͼ����
	int height; //ͼ��߶�
	int bytespp; //ÿ�����ص��ֽ���

	bool   load_rle_data(std::ifstream& in); //����ѹ����RLE(Run-Length Encoding)����
	bool unload_rle_data(std::ofstream& out); //��ͼ�����ݱ���ΪRLE��ʽ
public:
	enum Format {
		GRAYSCALE = 1, RGB = 3, RGBA = 4 //ͼ������ָ�ʽ, ��Ӧ��ÿ�������ֽ���Ϊ �Ҷ�(1) RGB(3) RGBA(4)
	};

	TGAImage();
	TGAImage(int w, int h, int bpp);
	TGAImage(const TGAImage& img);
	~TGAImage();

	bool read_tga_file(const char* filename); //���ļ���ȡTGAͼ������
	bool write_tga_file(const char* filename, bool rle = true); //��ͼ�����ݱ���ΪTGA�ļ�, ֧��ѡ���Ƿ�ʹ��RLEѹ��
	bool flip_horizontally(); //ˮƽ��תͼ��
	bool flip_vertically(); //��ֱ��תͼ��
	bool scale(int w, int h); //����ͼ���µĿ�Ⱥ͸߶�
	TGAColor get(int x, int y); //��ȡָ��λ�õ�������ɫ
	bool set(int x, int y, TGAColor& c); //����ָ��λ�õ�������ɫ
	bool set(int x, int y, const TGAColor& c);
	
	TGAImage& operator =(const TGAImage& img); //���ظ�ֵ�����
	int get_width();
	int get_height();
	int get_bytespp();
	unsigned char* buffer(); //����ͼ�����ݵ�ָ��
	void clear(); //���ͼ������
};

#endif //__IMAGE_H__