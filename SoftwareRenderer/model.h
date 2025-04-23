#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

//ģ����ÿ�������������������Ϊvector<Vec3i>, ÿ������3��Vec3i���, ������������
//ÿ������洢��������ֵ, �ֱ��Ӧ��������/������������/������������
class Model {
private:
	std::vector<std::vector<Vec3i> > faces_;
	std::vector<Vec3f> verts_; //�洢��������
	std::vector<Vec2f> uv_; //�洢��������
	std::vector<Vec3f> norms_; //�洢��������
	TGAImage diffuseMap_; //�洢ģ�͵�����������ͼ��
	TGAImage normalMap_; //�洢ģ�͵ķ�����ͼ, �����޸��������ķ��ߣ���ʵ�ָ����ӵı���ϸ��
	TGAImage specularMap_; //�洢ģ�͵ľ��淴����ͼ, ���ڿ���������治ͬ����ķ�������
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
