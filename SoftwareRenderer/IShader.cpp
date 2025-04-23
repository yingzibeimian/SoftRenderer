#include <cmath>
#include <limits>
#include "IShader.h"

using namespace std;

IShader::~IShader() {}

//FlatShaderʵ��
Vec4f FlatShader::vertex(int iface, int nthvert) {
	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	varying_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));

	return Viewport * Projection * ModelView * gl_vertex;
}

bool FlatShader::fragment(Vec3f weights, TGAColor& color) {
	//���������������ò��������������ķ���
	Vec3f n = cross(varying_tri.col(1) - varying_tri.col(0), varying_tri.col(2) - varying_tri.col(0)).normalize();
	float intensity = max(0.f, min(1.f, n * light_dir)); //������Ĺ���ǿ��
	color = TGAColor(255, 255, 255) * intensity;
	return false;
}



//GouraudShaderʵ��
Vec4f GouraudShader::vertex(int iface, int nthvert) {
	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert)); //תΪ�������
	gl_vertex = Viewport * Projection * ModelView * gl_vertex; //Ӧ��ģ����ͼ��ͶӰ���ӿڱ任

	//���ݶ��㷨������շ���ĵ�� ȷ��ÿ������Ĺ���ǿ��
	varying_intensity[nthvert] = max(0.f, model->normal(iface, nthvert) * light_dir);
	return gl_vertex;
}

bool GouraudShader::fragment(Vec3f weights, TGAColor& color) {
	float intensity = varying_intensity * weights; //ʹ�ù���ǿ�Ⱥ��������� ��ֵ���㵱ǰƬԪ/���صĹ���ǿ��
	color = TGAColor(255, 255, 255) * intensity; //���ݹ���ǿ�ȵ�����ɫ
	return false; //��������ƬԪ
}



//ToonShaderʵ��
Vec4f ToonShader::vertex(int iface, int nthvert) {
	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	gl_vertex = Viewport * Projection * ModelView * gl_vertex;

	varying_intensity[nthvert] = max(0.f, model->normal(iface, nthvert) * light_dir);
	return gl_vertex;
}

bool ToonShader::fragment(Vec3f weights, TGAColor& color) {
	float intensity = varying_intensity * weights;
	if (intensity > .85) {
		intensity = 1;
	}
	else if (intensity > .60) {
		intensity = .8;
	}
	else if (intensity > .45) {
		intensity = .6;
	}
	else if (intensity > .3) {
		intensity = .45;
	}
	else if (intensity > .15) {
		intensity = .3;
	}
	else {
		intensity = 0;
	}
	color = TGAColor(255, 155, 0) * intensity; //���ݹ���ǿ�ȵ�����ɫ
	return false; //��������ƬԪ
}

//TextureShaderʵ��
Vec4f TextureShader::vertex(int iface, int nthvert) {
	varying_uv.set_col(nthvert, model->uv(iface, nthvert)); //����ǰ�����uv����洢��varying_uv�ĵ�nthvert��
	varying_intensity[nthvert] = max(0.f, model->normal(iface, nthvert) * light_dir);

	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	return Viewport * Projection * ModelView * gl_vertex;
}

bool TextureShader::fragment(Vec3f weights, TGAColor& color) {
	float intensity = varying_intensity * weights;
	Vec2f uv = varying_uv * weights; //ʹ�õ�ǰ���ص���������weights���㵱ǰ���ص�uv����
	color = model->diffuse(uv) * intensity; //��ȡdiffuseMap_�е�������ɫֵ, ��*intensity
	return false;
}



//NormalMappingShaderʵ��
Vec4f NormalMappingShader::vertex(int iface, int nthvert) {
	varying_uv.set_col(nthvert, model->uv(iface, nthvert));

	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	return Viewport * Projection * ModelView * gl_vertex;
}

bool NormalMappingShader::fragment(Vec3f weights, TGAColor& color) {
	Vec2f uv = varying_uv * weights;
	Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize(); //�ӷ�����ͼ�ж�ȡ���߷���, ��ͨ��uniform_MIT�任����ͼ�ռ�
	Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); //ͨ��uniform_M����Դ����任����ͼ�ռ�
	float intensity = max(0.f, n * l); //ͨ���ӷ�����ͼ�����л�ȡ�ķ��� �� ��Դ���� ȷ�����������ǿ��
	color = model->diffuse(uv) * intensity;
	return false;
}



//PhongShaderʵ��
Vec4f PhongShader::vertex(int iface, int nthvert) {
	varying_uv.set_col(nthvert, model->uv(iface, nthvert));

	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	return Viewport * Projection * ModelView * gl_vertex;
}

bool PhongShader::fragment(Vec3f weights, TGAColor& color) {
	Vec2f uv = varying_uv * weights;
	Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
	Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
	Vec3f r = (n * (n * l * 2.f) - l).normalize(); //����ⷽ������r, ���Ը����ı��η��������Ƶ�
	//�߹ⷴ�����spec, r.z�Ƿ��䷽����z���ϵķ���, ��r.zС��0(���䷽��������), �����ǿ��Ϊ0, �������ǿ����r.z������
	//ʹ��ָ����������Ϊ��ģ�⾵�淴���(�����ȵ�)�߹�Ч��
	float spec = pow(max(r.z, 0.0f), model->specular(uv));
	float diff = max(0.f, n * l); //���������
	color = model->diffuse(uv);
	for (int i = 0; i < 3; i++) {
		color[i] = min<float>(5 + color[i] * (diff + .6 * spec), 255);
	}
	return false;
}


unique_ptr<IShader> createShader(const string& shaderType, Model* m, Matrix viewport, Matrix projection, Matrix modelview, Vec3f l) {
	if (shaderType == "Flat") {
		return make_unique<FlatShader>(m, viewport, projection, modelview, l);
	}
	else if (shaderType == "Gouraud") {
		return make_unique<GouraudShader>(m, viewport, projection, modelview, l);
	}
	else if (shaderType == "Toon") {
		return make_unique<ToonShader>(m, viewport, projection, modelview, l);
	}
	else if (shaderType == "Texture") {
		return make_unique<TextureShader>(m, viewport, projection, modelview, l);
	}
	else if (shaderType == "NormalMapping") {
		return make_unique<NormalMappingShader>(m, viewport, projection, modelview, l);
	}
	else if (shaderType == "Phong") {
		return make_unique<PhongShader>(m, viewport, projection, modelview, l);
	}
	else {
		throw invalid_argument("Invalid shader type: " + shaderType);
	}
}