#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <ctime>
#include "point_set.h"

typedef PointI PointI;

PointSet::PointSet()
{
    width_ = 0;
    height_ = 0;
    xmin_ = 0;
    xmax_ = 0;
    ymin_ = 0;
    ymax_ = 0;
}

PointSet::PointSet(int width, int height)
{
    width_ = width;
    height_ = height;
    map_.resize(width * height, -1);
    xmin_ = 0;
    xmax_ = 0;
    ymin_ = 0;
    ymax_ = 0;
}

PointSet::PointSet(const std::vector<PointI>& polygon)
{
    int size = polygon.size();
    if (size <= 1)
        return;

    // 这样首尾相接和首尾不相接的多边形都能正确被处理。
    // 其实应该在光栅化里做到这步？
    if (polygon[0] == polygon[size - 1])
        size--;

    if (size < 3)
        return;

    width_ = 0;
    height_ = 0;
    for (int i = 0; i < size; i++)
    {
        width_ = std::max(width_, polygon[i].x + 1);
        height_ = std::max(height_, polygon[i].y + 1);
    }
    map_.resize(width_ * height_, -1);
    xmin_ = 0;
    xmax_ = 0;
    ymin_ = 0;
    ymax_ = 0;

    // HW3_TODO：暴力算法改为AEL
    // http://staff.ustc.edu.cn/~lfdong/teach/acg2010-bk/chp7-2.pdf
    std::vector<int> intersects;
    for (int i = 0; i < height_; i++)
    {
        double y = i;
        intersects.clear();

        // 不考虑所有与顶点相交
        for (int j = 0; j < size; j++)
        {
            double x1 = polygon[j].x, x2 = polygon[(j + 1) % size].x;
            double y1 = polygon[j].y, y2 = polygon[(j + 1) % size].y;
            if ((y1 - y) * (y2 - y) < 0)
            {
                int x = (int)round((y - y1) * (x2 - x1) / (y2 - y1) + x1);
                intersects.push_back(x);
            }
        }
        // 再逐个与顶点判断计算
        for (int j = 0; j < size; j++)
        {
            if (y == polygon[j].y)
            {
                int y1 = polygon[(j - 1 + size) % size].y,
                    y2 = polygon[(j + 1) % size].y;
                if (y1 > y)
                    intersects.push_back(polygon[j].x);
                if (y2 > y)
                    intersects.push_back(polygon[j].x);
            }
        }

        sort(intersects.begin(), intersects.end());
        int size2 = intersects.size();
        assert(size2 % 2 == 0);
        for (int j = 0; j < size2; j += 2)
        {
            int begin_ = intersects[j], end_ = intersects[j + 1];
            for (int k = begin_; k <= end_; k++)
            {
                add_point(PointI(k, i));
            }
        }
    }

    // 预处理矩阵
    int size3 = point_list_.size();

    typedef Triplet<float> T;
    std::vector<T> tripletList;
    tripletList.reserve(size3 * 5);
    for (int i = 0; i < size3; i++)
    {
        PointI point = point_list_[i];
        PointSet::PointType type = check(point);
        if (type == kBoundary)
        {
            tripletList.push_back(T(i, i, 1));
        }
        else if (type == kIn)
        {
            tripletList.push_back(T(i, i, -4));
            tripletList.push_back(T(i, map_[point_to_1d_(point + PointI(1 , 0))], 1));
            tripletList.push_back(T(i, map_[point_to_1d_(point + PointI(-1, 0))], 1));
            tripletList.push_back(T(i, map_[point_to_1d_(point + PointI(0, 1 ))], 1));
            tripletList.push_back(T(i, map_[point_to_1d_(point + PointI(0, -1))], 1));
        }
    }

    matrix_ = SparseMatrix<float>(size3, size3);
    matrix_.setFromTriplets(tripletList.begin(), tripletList.end());
    split_.compute(matrix_);
}

PointSet::~PointSet()
{
}

int PointSet::point_to_1d_(PointI point)
{
    return width_ * point.y + point.x;
}

void PointSet::add_point(PointI point)
{
    if (point_list_.size() == 0)
    {
        xmin_ = xmax_ = point.x;
        ymin_ = ymax_ = point.y;
    }
    else
    {
        if (point.x < xmin_)
            xmin_ = point.x;
        if (point.x > xmax_)
            xmax_ = point.x;
        if (point.y < ymin_)
            ymin_ = point.y;
        if (point.y > ymax_)
            ymax_ = point.y;
    }
    map_[point_to_1d_(point)] = point_list_.size();
    point_list_.push_back(point);
}

bool PointSet::in_set(PointI point)
{
    if (point.x < 0 || point.x >= width_ || point.y < 0 || point.y >= height_)
        return false;
    return map_[point_to_1d_(point)] != -1;
}

PointSet::PointType PointSet::check(PointI point)
{
    if (!in_set(point))
        return kOut;
    if (   in_set(point + PointI(1 , 0)) 
        && in_set(point + PointI(-1, 0)) 
        && in_set(point + PointI(0, 1 )) 
        && in_set(point + PointI(0, -1)))
        return kIn;
    return kBoundary;
}

VectorXf PointSet::solve(VectorXf input)
{
    return split_.solve(input);
}

#undef point.x
#undef point.y