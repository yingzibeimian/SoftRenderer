#include <cmath>
#include <limits>
#include <cstdlib>
#include "rasterizer.h"

using namespace std;

const int depth = 255;

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

//视口变换矩阵, 将NDC坐标转为屏幕坐标 reference:https://www.songho.ca/opengl/gl_viewport.html
void viewport(int x, int y, int w, int h) {
	Viewport = Matrix::identity();
	Viewport[0][3] = x + w / 2.f;
	Viewport[1][3] = y + h / 2.f;
	Viewport[2][3] = depth / 2.f;

	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = depth / 2.f;
}

//透视投影变换矩阵
void projection(float coeff) {
	Projection = Matrix::identity();
	Projection[3][2] = coeff;
}

//模型变换(ModelView)矩阵, 作用是将世界坐标中的向量变换到相机空间(View Space)当中;
//其中eye为相机位置, center为相机指向的中心位置, up为世界坐标中的垂直向上向量(OpenGL中默认是(0,1,1), 即y轴, 一般保持不变)
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
	ModelView = Matrix::identity();

	Vec3f z = (eye - center).normalize(); //相机空间的z轴即从eye朝向center的向量
	Vec3f x = cross(up, z).normalize();
	Vec3f y = cross(z, x).normalize();

	for (int i = 0; i < 3; i++) {
		ModelView[0][i] = x[i];
		ModelView[1][i] = y[i];
		ModelView[2][i] = z[i];
		ModelView[i][3] = -center[i]; //ques
	}
}

//计算点p对于顶点a、b、c构成的三角形的重心坐标
Vec3f barycentric(Vec2f a, Vec2f b, Vec2f c, Vec2f p) {
	Vec3f vec1 = Vec3f(c[0] - a[0], b[0] - a[0], a[0] - p[0]);
	Vec3f vec2 = Vec3f(c[1] - a[1], b[1] - a[1], a[1] - p[1]);
	Vec3f u = cross(vec1, vec2);
	if (abs(u[2]) > 1e-2) {
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	}
	return Vec3f(-1, 1, 1); //如果u.z接近0, 即三角形面积为0, 说明三角形是退化的, 三个顶点可能或接近共线
}

//绘制三角形
void triangle(Vec4f* pts, std::unique_ptr<IShader>& shader, TGAImage& image, TGAImage& zbuffer) {
	//计算包围盒boundingbox
	Vec2f bboxmin(numeric_limits<float>::max(), numeric_limits<float>::max());
	Vec2f bboxmax(-numeric_limits<float>::max(), -numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1); //包围盒不超过图像边界
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = max(0.f, min(bboxmin[j], pts[i][j] / pts[i][3])); //注意这里需要做齐次除法Homogeneous Division, 将齐次坐标恢复到标准的非齐次坐标
			bboxmax[j] = min(clamp[j], max(bboxmax[j], pts[i][j] / pts[i][3]));
		}
	}
	Vec2i p; //遍历的像素点
	TGAColor color; //该像素点的颜色
	//遍历包围盒
	for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++) {
		for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++) {
			Vec3f weights = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(p)); //重心坐标权重
			float z = pts[0][2] * weights.x + pts[1][2] * weights.y + pts[2][2] * weights.z;
			float w = pts[0][3] * weights.x + pts[1][3] * weights.y + pts[2][3] * weights.z;
			int frag_depth = max(0, min(255, int(z / w + .5))); //片元深度值, 通过对齐次空间中的z和w做作为线性操作的插值, 再通过z/w将插值后的结果转回到屏幕空间, 恢复非线性关系
			//如果有重心坐标小于0, 说明该像素在三角形外部, 或是 深度插值未通过深度测试, 则无需渲染, 跳过
			if (weights.x < 0 || weights.y < 0 || weights.z < 0 || zbuffer.get(p.x, p.y)[0] > frag_depth) {
				continue;
			}
			bool discard = shader->fragment(weights, color);
			if (!discard) { //如果该像素不需要被抛弃
				zbuffer.set(p.x, p.y, TGAColor(frag_depth));
				image.set(p.x, p.y, color);
			}
		}
	}
}