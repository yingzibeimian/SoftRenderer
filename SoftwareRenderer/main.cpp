#include <vector>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "rasterizer.h"
#include "IShader.h"
using namespace std;

const int width = 800;
const int height = 800;
Model* model = nullptr;

Vec3f light_dir = Vec3f(1, 1, 1).normalize(); //���շ���
Vec3f eye(1, 1, 3); //���λ��
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

int main(int argc, char** argv) {
	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE); //�洢������Ȳ��ԵĻҶ�ͼ

	lookat(eye, center, up); //��ʼ��ModelViewģ�ͱ任����
	projection(-1.f / (eye - center).norm()); //��ʼ��͸��ͶӰ�任����
	//width/8 �� height/8 ʹ���ӿڵ�λ���������Ļ���Ͻ���һ����ƫ��, �Ӷ����ӱ߾ࡢ��ֹ�ڵ�,
	//width*3/4 �� height*3/4ʹ����Ⱦ�������, ��ģ�͸���Ŀռ���ʾ
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4); //��ʼ���ӿڱ任����

	string userInput;
	cout << "Enter shader type(Flat, Gouraud, Toon, Texture, NormalMapping, Phong): " << endl;
	cin >> userInput;
	unique_ptr<IShader> shader = createShader(userInput, model, Viewport, Projection, ModelView, light_dir);

	for (int i = 0; i < model->nfaces(); i++) {
		Vec4f screen_coords[3]; //��¼��Ļ����
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader->vertex(i, j); //���붥����ɫ��, ������Ļ����
		}
		triangle(screen_coords, shader, image, zbuffer);
	}

	image.flip_vertically(); // place the origin at the left bottom corner of the image
	zbuffer.flip_vertically();
	image.write_tga_file("output.tga");
	zbuffer.write_tga_file("zbuffer.tga");
	
	delete model;
	return 0;
}