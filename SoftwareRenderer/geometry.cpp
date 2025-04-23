#include "geometry.h"

//template <>表示对模板进行全特化, 后面的template <>则紧跟特化的具体类型
template <> template <> vec<3, int>::vec(const vec<3, float>& v) : x(int(v.x + .5f)), y(int(v.y + .5f)), z(int(v.z + .5f)) {}
template <> template <> vec<3, float>::vec(const vec<3, int>& v) : x(v.x), y(v.y), z(v.z) {}
template <> template <> vec<2, int>::vec(const vec<2, float>& v) : x(int(v.x + .5f)), y(int(v.y + .5f)) {}
template <> template <> vec<2, float>::vec(const vec<2, int>& v) : x(v.x), y(v.y) {}