#include <cmath>
#include <limits>
#include "IShader.h"

using namespace std;

IShader::~IShader() {}

//FlatShader实现
Vec4f FlatShader::vertex(int iface, int nthvert) {
	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	varying_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));

	return Viewport * Projection * ModelView * gl_vertex;
}

bool FlatShader::fragment(Vec3f weights, TGAColor& color) {
	//根据三个顶点利用叉积计算三角形面的法线
	Vec3f n = cross(varying_tri.col(1) - varying_tri.col(0), varying_tri.col(2) - varying_tri.col(0)).normalize();
	float intensity = max(0.f, min(1.f, n * light_dir)); //整个面的光照强度
	color = TGAColor(255, 255, 255) * intensity;
	return false;
}



//GouraudShader实现
Vec4f GouraudShader::vertex(int iface, int nthvert) {
	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert)); //转为齐次坐标
	gl_vertex = Viewport * Projection * ModelView * gl_vertex; //应用模型视图、投影、视口变换

	//根据顶点法线与光照方向的点积 确定每个顶点的光照强度
	varying_intensity[nthvert] = max(0.f, model->normal(iface, nthvert) * light_dir);
	return gl_vertex;
}

bool GouraudShader::fragment(Vec3f weights, TGAColor& color) {
	float intensity = varying_intensity * weights; //使用光照强度和重心坐标 插值计算当前片元/像素的光照强度
	color = TGAColor(255, 255, 255) * intensity; //根据光照强度调整颜色
	return false; //不丢弃该片元
}



//ToonShader实现
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
	color = TGAColor(255, 155, 0) * intensity; //根据光照强度调整颜色
	return false; //不丢弃该片元
}

//TextureShader实现
Vec4f TextureShader::vertex(int iface, int nthvert) {
	varying_uv.set_col(nthvert, model->uv(iface, nthvert)); //将当前顶点的uv坐标存储到varying_uv的第nthvert列
	varying_intensity[nthvert] = max(0.f, model->normal(iface, nthvert) * light_dir);

	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	return Viewport * Projection * ModelView * gl_vertex;
}

bool TextureShader::fragment(Vec3f weights, TGAColor& color) {
	float intensity = varying_intensity * weights;
	Vec2f uv = varying_uv * weights; //使用当前像素的重心坐标weights计算当前像素的uv坐标
	color = model->diffuse(uv) * intensity; //读取diffuseMap_中的纹理颜色值, 并*intensity
	return false;
}



//NormalMappingShader实现
Vec4f NormalMappingShader::vertex(int iface, int nthvert) {
	varying_uv.set_col(nthvert, model->uv(iface, nthvert));

	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	return Viewport * Projection * ModelView * gl_vertex;
}

bool NormalMappingShader::fragment(Vec3f weights, TGAColor& color) {
	Vec2f uv = varying_uv * weights;
	Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize(); //从法线贴图中读取法线方向, 并通过uniform_MIT变换到视图空间
	Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); //通过uniform_M将光源方向变换到视图空间
	float intensity = max(0.f, n * l); //通过从法线贴图纹理中获取的法线 和 光源方向 确定漫反射光照强度
	color = model->diffuse(uv) * intensity;
	return false;
}



//PhongShader实现
Vec4f PhongShader::vertex(int iface, int nthvert) {
	varying_uv.set_col(nthvert, model->uv(iface, nthvert));

	Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
	return Viewport * Projection * ModelView * gl_vertex;
}

bool PhongShader::fragment(Vec3f weights, TGAColor& color) {
	Vec2f uv = varying_uv * weights;
	Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
	Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
	Vec3f r = (n * (n * l * 2.f) - l).normalize(); //反射光方向向量r, 可以根据四边形法则自行推导
	//高光反射分量spec, r.z是反射方向在z轴上的分量, 若r.z小于0(反射方向背离视线), 则镜面光强度为0, 否则镜面光强度与r.z成正比
	//使用指数函数则是为了模拟镜面反射的(不均匀的)高光效果
	float spec = pow(max(r.z, 0.0f), model->specular(uv));
	float diff = max(0.f, n * l); //漫反射分量
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