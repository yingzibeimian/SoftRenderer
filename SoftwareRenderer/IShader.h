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

	Vec3f varying_intensity; //存储三角形每个顶点的光照强度
	mat<3, 3, float> varying_tri; //存储三角形每个顶点的三维坐标
	mat<2, 3, float> varying_uv; //存储三角形每个顶点的uv纹理坐标
	mat<4, 4, float> uniform_M = Projection * ModelView; //不需要乘以视口变换矩阵Viewport, Viewport的作用是将齐次裁剪坐标NDC映射到屏幕坐标
	mat<4, 4, float> uniform_MIT = uniform_M.invert_transpose(); //通过逆转置矩阵来抵消矩阵中的缩放影响

	IShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : model(m),  Viewport(vp), Projection(pj), ModelView(mv), light_dir(l) {}

	virtual ~IShader();
	virtual Vec4f vertex(int iface, int nthvert) = 0;
	virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

//逐面着色 FlatShading
struct FlatShader : public IShader {
	FlatShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//高洛德着色 GouraudShading
struct GouraudShader : public IShader {
	GouraudShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//卡通着色
struct ToonShader : public IShader {
	ToonShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//纹理贴图
struct TextureShader : public IShader {
	TextureShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//法线贴图, 效果比通过重心坐标对顶点光照强度做插值计算 更好
struct NormalMappingShader : public IShader {
	NormalMappingShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//加入镜面反射贴图, 采用Phong模型：Ambient + Diffuse + Specular = Phong Reflection
struct PhongShader : public IShader {
	PhongShader(Model* m, Matrix vp, Matrix pj, Matrix mv, Vec3f l) : IShader(m, vp, pj, mv, l) {}
	virtual Vec4f vertex(int iface, int nthvert) override;
	virtual bool fragment(Vec3f weights, TGAColor& color) override;
};

//Shader生成工厂, 通过用户输入决定实例化哪个Shader
std::unique_ptr<IShader> createShader(const std::string& shaderType, Model* m, Matrix viewport, Matrix projection, Matrix modelview, Vec3f l);

#endif __ISHADER_H__