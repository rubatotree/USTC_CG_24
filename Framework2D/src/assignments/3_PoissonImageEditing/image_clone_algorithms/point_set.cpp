#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <ctime>
#include "point_set.h"


namespace USTC_CG
{
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
    // Part0: 初始化
    width_ = 0;
    height_ = 0;
    xmin_ = INT_MAX;
    xmax_ = -INT_MAX;
    ymin_ = INT_MAX;
    ymax_ = -INT_MAX;

    int size_vertice = polygon.size();

    if (size_vertice < 3)
        return;

    for (int i = 0; i < size_vertice; i++)
    {
        xmin_ = std::min(xmin_, polygon[i].x);
        ymin_ = std::min(ymin_, polygon[i].y);
        xmax_ = std::max(xmax_, polygon[i].x);
        ymax_ = std::max(ymax_, polygon[i].y);
    }
    width_ = xmax_ + 1;
    height_ = ymax_ + 1;

    map_.resize(width_ * height_, -1);
    
    // Part1: 扫描线光栅化
    // AEL 有序边表光栅化算法
    // http://staff.ustc.edu.cn/~lfdong/teach/acg2010-bk/chp7-2.pdf

    et_.resize(ymax_ + 1, nullptr);
    ael_head_ = nullptr;

    for (int j = 0; j < size_vertice; j++)
    {
        PointI p1 = polygon[j], p2 = polygon[(j + 1) % size_vertice];
        if (p1.y == p2.y)
            continue;
        if (p1.y > p2.y)
            std::swap(p1, p2);
        et_insert(p1.y, new ETElement(p1, p2));
    }

    int y = ymin_;
    do
    {
        for (ETElement* itr = et_[y]; itr != nullptr; itr = itr->nxt)
        {
            ael_insert(new ETElement(itr->ymax, itr->x, itr->dx, nullptr));
        }
        bool fill = false;
        for (ETElement* itr = ael_head_; itr != nullptr; itr = itr->nxt)
        {
            fill = !fill;
            if (fill)
            {
                if (itr->nxt == nullptr)
                    break;
                // 四舍五入取整，闭区间
                int x1 = round(itr->x), x2 = round(itr->nxt->x);
                for (int x = x1; x <= x2; x++)
                {
                    PointI point = PointI(x, y);
                    map_[point_to_1d_(point)] = point_list_.size();
                    point_list_.push_back(point);
                }
            }
        }
        
        y++;
        if (y > ymax_)
            break;

        while (ael_head_ != nullptr && ael_head_->ymax == y)
        {
            auto tmp = ael_head_;
            ael_head_ = ael_head_->nxt;
            delete tmp;
        }
        for (ETElement* itr = ael_head_; itr != nullptr && itr->nxt != nullptr; itr = itr->nxt)
        {
            while (itr->nxt != nullptr && itr->nxt->ymax == y)
            {
                auto tmp = itr->nxt;
                itr->nxt = itr->nxt->nxt;
                delete tmp;
            }
        }
        
        // 重排序
        ETElement* tmp_head_ = ael_head_;
        ael_head_ = nullptr;
        for (ETElement* itr = tmp_head_; itr != nullptr; itr = itr->nxt)
        {
            ael_insert(
                new ETElement(itr->ymax, itr->x + itr->dx, itr->dx, nullptr));
        }
        for (ETElement* itr = tmp_head_; itr != nullptr;)
        {
            auto tmp = itr;
            itr = itr->nxt;
            delete tmp;
        }
    }
    while (y <= ymax_);

    for (ETElement* itr = ael_head_; itr != nullptr; )
    {
        auto tmp = itr;
        itr = itr->nxt;
        delete tmp;
    }
    for (int y = ymin_; y <= ymax_; y++)
    {
        for (ETElement* itr = et_[y]; itr != nullptr; )
        {
            auto tmp = itr;
            itr = itr->nxt;
            delete tmp;
        }
    }

    if (point_list_.size() == 0)
        return;

    // Part2: 预处理矩阵
    int size_points = point_list_.size();

    typedef Triplet<float> T;
    std::vector<T> tripletList;
    tripletList.reserve(size_points * 5);
    for (int i = 0; i < size_points; i++)
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
            tripletList.push_back(
                T(i, map_[point_to_1d_(point + PointI(1, 0))], 1));
            tripletList.push_back(
                T(i, map_[point_to_1d_(point + PointI(-1, 0))], 1));
            tripletList.push_back(
                T(i, map_[point_to_1d_(point + PointI(0, 1))], 1));
            tripletList.push_back(
                T(i, map_[point_to_1d_(point + PointI(0, -1))], 1));
        }
    }

    matrix_ = SparseMatrix<float>(size_points, size_points);
    matrix_.setFromTriplets(tripletList.begin(), tripletList.end());
    solver_.compute(matrix_);
    status_ = true;
}

PointSet::~PointSet()
{
}

void PointSet::et_insert(int y, ETElement* obj)
{
    if (et_[y] == nullptr)
    {
        et_[y] = obj;
    }
    else if (obj->x < et_[y]->x ||
                (obj->x == et_[y]->x && obj->dx < et_[y]->dx))
    {
        obj->nxt = et_[y];
        et_[y] = obj;
    }
    else
    {
        ETElement* itr = et_[y];
        for (; itr->nxt != nullptr; itr = itr->nxt)
        {
            if (obj->x < itr->nxt->x ||
                (obj->x == itr->nxt->x && obj->dx < itr->nxt->dx))
                break;
        }
        obj->nxt = itr->nxt;
        itr->nxt = obj;
    }
}

void PointSet::ael_insert(ETElement* obj)
{
    if (ael_head_ == nullptr)
    {
        ael_head_ = obj;
    }
    else if (
        obj->x < ael_head_->x || (obj->x == ael_head_->x && obj->dx < ael_head_->dx))
    {
        obj->nxt = ael_head_;
        ael_head_ = obj;
    }
    else
    {
        ETElement* itr = ael_head_;
        for (; itr->nxt != nullptr; itr = itr->nxt)
        {
            if (obj->x < itr->nxt->x || (obj->x == itr->nxt->x && obj->dx < itr->nxt->dx))
                break;
        }
        obj->nxt = itr->nxt;
        itr->nxt = obj;
    }
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
    if (in_set(point + PointI(1, 0)) && in_set(point + PointI(-1, 0)) &&
        in_set(point + PointI(0, 1)) && in_set(point + PointI(0, -1)))
        return kIn;
    return kBoundary;
}

VectorXf PointSet::solve(VectorXf input)
{
    return solver_.solve(input);
}
}  // namespace USTC_CG