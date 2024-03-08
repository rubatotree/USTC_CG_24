#pragma once
#include <cmath>
namespace USTC_CG
{
// 自己写的计算几何库
struct Vector
{
    float x, y;
    Vector(float x = 0, float y = 0) : x(x), y(y)
    {
    }
    Vector operator+(Vector right) const
    {
        return Vector(x + right.x, y + right.y);
    }
    Vector operator-(Vector right) const
    {
        return Vector(x - right.x, y - right.y);
    }
    Vector operator*(float val) const
    {
        return Vector(x * val, y * val);
    }
    Vector operator/(float val) const
    {
        return Vector(x / val, y / val);
    }
    float operator*(Vector right) const
    {
        return x * right.x + y * right.y;
    }
    float operator^(Vector right) const
    {
        return x * right.y - y * right.x;
    }
    float length_sqr()
    {
        return (*this) * (*this);
    }
    float length()
    {
        return sqrt(length_sqr());
    }
    Vector normalize()
    {
        return Vector(x, y) / length();
    }
};
struct Point
{
    float x, y;
    Point(float x = 0, float y = 0) : x(x), y(y)
    {
    }
    Point operator+(Vector right) const
    {
        return Point(x + right.x, y + right.y);
    }
    Vector operator-(Point right) const
    {
        return Vector(x - right.x, y - right.y);
    }
    Point operator*(float val) const
    {
        return Point(x * val, y * val);
    }
    Point operator/(float val) const
    {
        return Point(x / val, y / val);
    }
};
struct Matrix22
{
    Vector base[2];
    Matrix22(Vector base1, Vector base2)
    {
        base[0] = base1;
        base[1] = base2;
    }
    Matrix22(float a, float b, float c, float d)
    {
        base[0] = Vector(a, c);
        base[1] = Vector(b, d);
    }
    Matrix22 operator+(Matrix22 right) const
    {
        return Matrix22(base[0] + right.base[0], base[1] + right.base[1]);
    }
    Vector operator*(Vector right) const
    {
        return base[0] * right.x + base[1] * right.y;
    }
    Matrix22 operator*(float val) const
    {
        return Matrix22(base[0] * val, base[1] * val);
    }
    Matrix22 operator/(float val) const
    {
        return Matrix22(base[0] / val, base[1] / val);
    }
    float det()
    {
        return base[0] ^ base[1];
    }
    Matrix22 inv()
    {
        return Matrix22(base[1].y, -base[1].x, -base[0].y, base[0].x) / det();
    }
};
}  // namespace USTC_CG