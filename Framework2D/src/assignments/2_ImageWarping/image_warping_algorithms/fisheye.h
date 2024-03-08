#pragma once

#include <Eigen/Dense>

#include "image_warping.h"

namespace USTC_CG
{
class WarpingFisheye : public ImageWarpingAlgorithm
{
   public:
    WarpingFisheye(){};
    WarpingFisheye(int width, int height) : width_(width), height_(height){};
    ~WarpingFisheye(){};
    Point Transform(Point src);
    void Update();
    void SetParams(int width, int height)
    {
        width_ = width;
        height_ = height;
    }

   private:
    int width_ = 512, height_ = 512;
};
}  // namespace USTC_CG