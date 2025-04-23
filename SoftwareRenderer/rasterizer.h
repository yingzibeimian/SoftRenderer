#ifndef __RASTERIZER_H__
#define __RASTERIZER_H__

#include "tgaimage.h"
#include "geometry.h"
#include "IShader.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); //coeff = -1 / c
void lookat(Vec3f eye, Vec3f center, Vec3f up);
Vec3f barycentric(Vec2f a, Vec2f b, Vec2f c, Vec2f p);
void triangle(Vec4f* pts, std::unique_ptr<IShader>& shader, TGAImage& image, TGAImage& zbuffer);

#endif __RASTERIZER_H__