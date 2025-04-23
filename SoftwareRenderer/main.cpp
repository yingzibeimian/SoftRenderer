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

Vec3f light_dir = Vec3f(1, 1, 1).normalize(); //光照方向
Vec3f eye(1, 1, 3); //相机位置
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
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE); //存储用于深度测试的灰度图

	lookat(eye, center, up); //初始化ModelView模型变换矩阵
	projection(-1.f / (eye - center).norm()); //初始化透视投影变换矩阵
	//width/8 和 height/8 使得视口的位置相对于屏幕左上角有一定的偏移, 从而增加边距、防止遮挡,
	//width*3/4 和 height*3/4使得渲染区域更大, 给模型更多的空间显示
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4); //初始化视口变换矩阵

	string userInput;
	cout << "Enter shader type(Flat, Gouraud, Toon, Texture, NormalMapping, Phong): " << endl;
	cin >> userInput;
	unique_ptr<IShader> shader = createShader(userInput, model, Viewport, Projection, ModelView, light_dir);

	for (int i = 0; i < model->nfaces(); i++) {
		Vec4f screen_coords[3]; //记录屏幕坐标
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader->vertex(i, j); //传入顶点着色器, 处理屏幕坐标
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