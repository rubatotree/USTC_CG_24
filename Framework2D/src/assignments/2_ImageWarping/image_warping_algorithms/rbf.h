#pragma once

#include <Eigen/Dense>

#include "image_warping.h"
namespace USTC_CG
{
class WarpingRBF : public ImageWarpingAlgorithm
{
   public:
    WarpingRBF(){};
    WarpingRBF(double r, double mu)
    {
        SetParams(r, mu);
    };
    ~WarpingRBF(){};
    Point Transform(Point src);
    void Update();
    void SetParams(double r, double mu)
    {
        r_ = r;
        mu_ = mu;
    }

   private:
    double r_ = 30;
    double mu_ = -1;
    int dist(int i, int j);
    double func(int base, Point src);
    Eigen::VectorXd AlphaX, AlphaY;
};
}  // namespace USTC_CG