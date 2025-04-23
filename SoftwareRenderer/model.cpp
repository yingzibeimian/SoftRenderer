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
        std::getline(in, line); //逐行读取文件内容, 将每一行存入line字符串中
        std::istringstream iss(line.c_str()); //将line转换为输入字符串流iss
        char trash;
        if (!line.compare(0, 2, "v ")) { //处理顶点数据 (如果line的前两个字符为"v ", compare返回0)
            iss >> trash; //读取并丢弃第一个字符"v"
            Vec3f v; //通过三维向量Vec3f类型的变量v来存储顶点数据
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 3, "vt ")) { //处理纹理坐标数据
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
        }
        else if (!line.compare(0, 3, "vn ")) { //处理法线向量数据
            iss >> trash >> trash;
            Vec3f normal;
            for (int i = 0; i < 3; i++) iss >> normal[i];
            norms_.push_back(normal);
        }
        else if (!line.compare(0, 2, "f ")) { //处理面数据
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash; //读取并丢弃第一个字符"f"
            //obj文件中面数据的格式通常是v/vt/vn (顶点索引/纹理坐标索引/法线索引)
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; i++) tmp[i]--;
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
    loadTexture(filename, "_diffuse.tga", diffuseMap_); //将纹理文件读入diffuseMap_
    loadTexture(filename, "_nm.tga", normalMap_);
    loadTexture(filename, "_spec.tga", specularMap_);
}

Model::~Model() {
}

//返回模型中的顶点数量
int Model::nverts() {
    return (int)verts_.size();
}

//返回模型中的面数量
int Model::nfaces() {
    return (int)faces_.size();
}

//返回给定面idx中所有顶点的数据索引
std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i = 0; i < faces_[idx].size(); i++) {
        face.push_back(faces_[idx][i][0]);
    }
    return face;
}

//返回第i个顶点的数据
Vec3f Model::vert(int i) {
    return verts_[i];
}

//返回第i个面的第n个顶点的数据
Vec3f Model::vert(int iface, int nthvert) {
    return verts_[faces_[iface][nthvert][0]];
}

//根据面和顶点索引, 获取与该顶点相关联的纹理坐标
Vec2f Model::uv(int iface, int nthvert) {
    return uv_[faces_[iface][nthvert][1]];
    //int idx = faces_[iface][nthvert][1];
    //return Vec2i(uv_[idx].x * diffuseMap_.get_width(), uv_[idx].y * diffuseMap_.get_height()); //obj文件中的纹理坐标为标准化的[0,1], 通过*宽/高映射到实际的图像尺寸
}

//根据面和顶点索引, 获取与该顶点的法线向量
Vec3f Model::normal(int iface, int nthvert) {
    int idx = faces_[iface][nthvert][2];
    return norms_[idx].normalize();
}

//根据二维纹理坐标uv从normalMap_中获取对应的法线向量
Vec3f Model::normal(Vec2f uvf) {
    Vec2i uv(uvf[0] * normalMap_.get_width(), uvf[1] * normalMap_.get_height());
    TGAColor c = normalMap_.get(uv[0], uv[1]);
    Vec3f res;
    for (int i = 0; i < 3; i++) {
        res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f; //将[0,255]范围内的BGR归一化到[-1, 1]内
    }
    return res;
}

//通过二维纹理坐标uv从diffuseMap_中获取对应的颜色值
TGAColor Model::diffuse(Vec2f uvf) {
    Vec2i uv(uvf[0] * diffuseMap_.get_width(), uvf[1] * diffuseMap_.get_height());
    return diffuseMap_.get(uv.x, uv.y);
}

//通过二维纹理坐标uv从specularMap_中获取对应的镜面反射系数(灰度值)
float Model::specular(Vec2f uvf) {
    Vec2i uv(uvf[0] * specularMap_.get_width(), uvf[1] * specularMap_.get_height());
    return specularMap_.get(uv[0], uv[1])[0] / 1.f;
}

//读取纹理文件并将其存储在TGAImage对象image当中, suffix为纹理文件后缀
void Model::loadTexture(std::string filename, const char* suffix, TGAImage& image) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of("."); //例如obj/african_head.obj
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix); //obj/african_head + _diffuse.tga = obj/african_head_diffuse.tga
        std::cerr << "file " << texfile << " loading " << (image.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        image.flip_vertically();
    }
}