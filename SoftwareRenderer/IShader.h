#ifndef __ISHADER_H__
#define __ISHADER_H__

#include <memory>
#include "geometry.h"
#include "model.h"

struct IShader {
	Model* model;
	Matrix Viewport;
	Matrix Projection;
	Matrix ModelView;
	Vec3f light_dir;

	Vec3f varying_intensity; //�洢������ÿ������Ĺ���ǿ��
	mat<3, 3, float> varying_tri; //�洢������ÿ���������ά����
	mat<2, 3, float> varying_uv; //�洢������ÿ�������uv��������
	mat<4, 4, float> uniform_M = Projection * ModelView; //����Ҫ�����ӿڱ任����Viewport, Viewport�������ǽ���βü�����NDCӳ�䵽��Ļ����
	mat<4, 4, float> uniform_MIT = uniform_M.invert_transpose(); //ͨ����ת�þ��������������е�����Ӱ��

	IShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : model(m),  Viewport(vp), Projection(pj), ModelView(mv), light_dir(l) {}

	virtual ~IShader();
	virtual Vec4f vertex(int iface, int nthvert) = 0;
	virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

//������ɫ FlatShading
struct FlatShader : public IShader {
	FlatShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//�������ɫ GouraudShading
struct GouraudShader : public IShader {
	GouraudShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//��ͨ��ɫ
struct ToonShader : public IShader {
	ToonShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//������ͼ
struct TextureShader : public IShader {
	TextureShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//������ͼ, Ч����ͨ����������Զ������ǿ������ֵ���� ����
struct NormalMappingShader : public IShader {
	NormalMappingShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//���뾵�淴����ͼ, ����Phongģ�ͣ�Ambient + Diffuse + Specular = Phong Reflection
struct PhongShader : public IShader {
	PhongShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//Shader���ɹ���, ͨ���û��������ʵ�����ĸ�Shader
std::unique_ptr<IShader> createShader(const std::string& shaderType, Model* m, Matrix viewport, Matrix projection, Matrix modelview, Vec3f l);

#endif __ISHADER_H__