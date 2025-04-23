#include <cmath>
#include <limits>
#include <cstdlib>
#include "rasterizer.h"

using namespace std;

const int depth = 255;

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

//�ӿڱ任����, ��NDC����תΪ��Ļ���� reference:https://www.songho.ca/opengl/gl_viewport.html
void viewport(int x, int y, int w, int h) {
	Viewport = Matrix::identity();
	Viewport[0][3] = x + w / 2.f;
	Viewport[1][3] = y + h / 2.f;
	Viewport[2][3] = depth / 2.f;

	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = depth / 2.f;
}

//͸��ͶӰ�任����
void projection(float coeff) {
	Projection = Matrix::identity();
	Projection[3][2] = coeff;
}

//ģ�ͱ任(ModelView)����, �����ǽ����������е������任������ռ�(View Space)����;
//����eyeΪ���λ��, centerΪ���ָ�������λ��, upΪ���������еĴ�ֱ��������(OpenGL��Ĭ����(0,1,1), ��y��, һ�㱣�ֲ���)
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
	ModelView = Matrix::identity();

	Vec3f z = (eye - center).normalize(); //����ռ��z�ἴ��eye����center������
	Vec3f x = cross(up, z).normalize();
	Vec3f y = cross(z, x).normalize();

	for (int i = 0; i < 3; i++) {
		ModelView[0][i] = x[i];
		ModelView[1][i] = y[i];
		ModelView[2][i] = z[i];
		ModelView[i][3] = -center[i]; //ques
	}
}

//�����p���ڶ���a��b��c���ɵ������ε���������
Vec3f barycentric(Vec2f a, Vec2f b, Vec2f c, Vec2f p) {
	Vec3f vec1 = Vec3f(c[0] - a[0], b[0] - a[0], a[0] - p[0]);
	Vec3f vec2 = Vec3f(c[1] - a[1], b[1] - a[1], a[1] - p[1]);
	Vec3f u = cross(vec1, vec2);
	if (abs(u[2]) > 1e-2) {
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	}
	return Vec3f(-1, 1, 1); //���u.z�ӽ�0, �����������Ϊ0, ˵�����������˻���, ����������ܻ�ӽ�����
}

//����������
void triangle(Vec4f* pts, std::unique_ptr<IShader>& shader, TGAImage& image, TGAImage& zbuffer) {
	//�����Χ��boundingbox
	Vec2f bboxmin(numeric_limits<float>::max(), numeric_limits<float>::max());
	Vec2f bboxmax(-numeric_limits<float>::max(), -numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1); //��Χ�в�����ͼ��߽�
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = max(0.f, min(bboxmin[j], pts[i][j] / pts[i][3])); //ע��������Ҫ����γ���Homogeneous Division, ���������ָ�����׼�ķ��������
			bboxmax[j] = min(clamp[j], max(bboxmax[j], pts[i][j] / pts[i][3]));
		}
	}
	Vec2i p; //���������ص�
	TGAColor color; //�����ص����ɫ
	//������Χ��
	for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++) {
		for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++) {
			Vec3f weights = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(p)); //��������Ȩ��
			float z = pts[0][2] * weights.x + pts[1][2] * weights.y + pts[2][2] * weights.z;
			float w = pts[0][3] * weights.x + pts[1][3] * weights.y + pts[2][3] * weights.z;
			int frag_depth = max(0, min(255, int(z / w + .5))); //ƬԪ���ֵ, ͨ������οռ��е�z��w����Ϊ���Բ����Ĳ�ֵ, ��ͨ��z/w����ֵ��Ľ��ת�ص���Ļ�ռ�, �ָ������Թ�ϵ
			//�������������С��0, ˵�����������������ⲿ, ���� ��Ȳ�ֵδͨ����Ȳ���, ��������Ⱦ, ����
			if (weights.x < 0 || weights.y < 0 || weights.z < 0 || zbuffer.get(p.x, p.y)[0] > frag_depth) {
				continue;
			}
			bool discard = shader->fragment(weights, color);
			if (!discard) { //��������ز���Ҫ������
				zbuffer.set(p.x, p.y, TGAColor(frag_depth));
				image.set(p.x, p.y, color);
			}
		}
	}
}