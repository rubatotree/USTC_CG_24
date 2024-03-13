#pragma once

#include <Eigen/Sparse>
#include <vector>
#include <tuple>
using namespace Eigen;

namespace USTC_CG
{
// 整点
struct PointI
{
    int x, y;
    PointI(int x = 0, int y = 0) : x(x), y(y)
    {
    }
    PointI operator+(PointI right) const
    {
        return PointI(x + right.x, y + right.y);
    }
    PointI operator-(PointI right) const
    {
        return PointI(x - right.x, y - right.y);
    }
    PointI operator*(int val) const
    {
        return PointI(x * val, y * val);
    }
    PointI operator/(int val) const
    {
        return PointI(x / val, y / val);
    }
    bool operator==(const PointI& right) const
    {
        return x == right.x && y == right.y;
    }
};

// 点集类
class PointSet
{
   public:
    enum PointType
    {
        kIn,
        kOut,
        kBoundary
    };
    PointSet();
    PointSet(int width, int height);
    PointSet(
        const std::vector<PointI>&
            polygon);  // 该构造函数接受一个多边形，并光栅化确定点集内元素。
    ~PointSet();
    void add_point(PointI point);
    PointType check(PointI point);
    PointI get_point_at(int index)
    {
        return point_list_[index];
    }
    int get_width_()
    {
        return xmax_ - xmin_;
    }
    int get_height_()
    {
        return ymax_ - ymin_;
    }
    size_t size()
    {
        return point_list_.size();
    }
    VectorXf solve(VectorXf input);

   private:
    int xmin_, xmax_, ymin_, ymax_;
    int width_, height_;
    std::vector<int> map_;
    std::vector<PointI> point_list_;
    SparseMatrix<float> matrix_;
    SparseLU<SparseMatrix<float>, COLAMDOrdering<int>> split_;
    int point_to_1d_(PointI point);
    bool in_set(PointI point);
};
}  // namespace USTC_CG