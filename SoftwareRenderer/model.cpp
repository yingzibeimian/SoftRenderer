#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) : verts_(), faces_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line); //���ж�ȡ�ļ�����, ��ÿһ�д���line�ַ�����
        std::istringstream iss(line.c_str()); //��lineת��Ϊ�����ַ�����iss
        char trash;
        if (!line.compare(0, 2, "v ")) { //���������� (���line��ǰ�����ַ�Ϊ"v ", compare����0)
            iss >> trash; //��ȡ��������һ���ַ�"v"
            Vec3f v; //ͨ����ά����Vec3f���͵ı���v���洢��������
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 3, "vt ")) { //����������������
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
        }
        else if (!line.compare(0, 3, "vn ")) { //��������������
            iss >> trash >> trash;
            Vec3f normal;
            for (int i = 0; i < 3; i++) iss >> normal[i];
            norms_.push_back(normal);
        }
        else if (!line.compare(0, 2, "f ")) { //����������
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash; //��ȡ��������һ���ַ�"f"
            //obj�ļ��������ݵĸ�ʽͨ����v/vt/vn (��������/������������/��������)
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; i++) tmp[i]--;
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
    loadTexture(filename, "_diffuse.tga", diffuseMap_); //�������ļ�����diffuseMap_
    loadTexture(filename, "_nm.tga", normalMap_);
    loadTexture(filename, "_spec.tga", specularMap_);
}

Model::~Model() {
}

//����ģ���еĶ�������
int Model::nverts() {
    return (int)verts_.size();
}

//����ģ���е�������
int Model::nfaces() {
    return (int)faces_.size();
}

//���ظ�����idx�����ж������������
std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i = 0; i < faces_[idx].size(); i++) {
        face.push_back(faces_[idx][i][0]);
    }
    return face;
}

//���ص�i�����������
Vec3f Model::vert(int i) {
    return verts_[i];
}

//���ص�i����ĵ�n�����������
Vec3f Model::vert(int iface, int nthvert) {
    return verts_[faces_[iface][nthvert][0]];
}

//������Ͷ�������, ��ȡ��ö������������������
Vec2f Model::uv(int iface, int nthvert) {
    return uv_[faces_[iface][nthvert][1]];
    //int idx = faces_[iface][nthvert][1];
    //return Vec2i(uv_[idx].x * diffuseMap_.get_width(), uv_[idx].y * diffuseMap_.get_height()); //obj�ļ��е���������Ϊ��׼����[0,1], ͨ��*��/��ӳ�䵽ʵ�ʵ�ͼ��ߴ�
}

//������Ͷ�������, ��ȡ��ö���ķ�������
Vec3f Model::normal(int iface, int nthvert) {
    int idx = faces_[iface][nthvert][2];
    return norms_[idx].normalize();
}

//���ݶ�ά��������uv��normalMap_�л�ȡ��Ӧ�ķ�������
Vec3f Model::normal(Vec2f uvf) {
    Vec2i uv(uvf[0] * normalMap_.get_width(), uvf[1] * normalMap_.get_height());
    TGAColor c = normalMap_.get(uv[0], uv[1]);
    Vec3f res;
    for (int i = 0; i < 3; i++) {
        res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f; //��[0,255]��Χ�ڵ�BGR��һ����[-1, 1]��
    }
    return res;
}

//ͨ����ά��������uv��diffuseMap_�л�ȡ��Ӧ����ɫֵ
TGAColor Model::diffuse(Vec2f uvf) {
    Vec2i uv(uvf[0] * diffuseMap_.get_width(), uvf[1] * diffuseMap_.get_height());
    return diffuseMap_.get(uv.x, uv.y);
}

//ͨ����ά��������uv��specularMap_�л�ȡ��Ӧ�ľ��淴��ϵ��(�Ҷ�ֵ)
float Model::specular(Vec2f uvf) {
    Vec2i uv(uvf[0] * specularMap_.get_width(), uvf[1] * specularMap_.get_height());
    return specularMap_.get(uv[0], uv[1])[0] / 1.f;
}

//��ȡ�����ļ�������洢��TGAImage����image����, suffixΪ�����ļ���׺
void Model::loadTexture(std::string filename, const char* suffix, TGAImage& image) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of("."); //����obj/african_head.obj
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix); //obj/african_head + _diffuse.tga = obj/african_head_diffuse.tga
        std::cerr << "file " << texfile << " loading " << (image.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        image.flip_vertically();
    }
}