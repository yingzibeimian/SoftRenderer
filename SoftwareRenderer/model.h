#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

//模型中每个三角形面的数据类型为vector<Vec3i>, 每个面由3个Vec3i组成, 代表三个顶点
//每个顶点存储三个整数值, 分别对应顶点索引/纹理坐标索引/法线向量索引
class Model {
private:
	std::vector<std::vector<Vec3i> > faces_;
	std::vector<Vec3f> verts_; //存储顶点坐标
	std::vector<Vec2f> uv_; //存储纹理坐标
	std::vector<Vec3f> norms_; //存储法线向量
	TGAImage diffuseMap_; //存储模型的漫反射纹理图像
	TGAImage normalMap_; //存储模型的法线贴图, 用于修改物体表面的法线，以实现更复杂的表面细节
	TGAImage specularMap_; //存储模型的镜面反射贴图, 用于控制物体表面不同区域的反射光泽度
	void loadTexture(std::string filename, const char* suffix, TGAImage& image);

public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	std::vector<int> face(int idx);
	Vec3f vert(int i);
	Vec3f vert(int iface, int nthvert);
	Vec2f uv(int iface, int nthvert);
	Vec3f normal(int iface, int nthvert);
	Vec3f normal(Vec2f uv);
	TGAColor diffuse(Vec2f uv);
	float specular(Vec2f uv);
};

#endif //__MODEL_H__
