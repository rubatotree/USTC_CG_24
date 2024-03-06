#pragma once

#include <Eigen/Dense>
#include "image_warping.h"

class WarpingIDW : public ImageWarpingAlgorithm
{
   public:
    WarpingIDW(){};
    WarpingIDW(float mu)
    {
        mu_ = mu;
    };
    ~WarpingIDW(){};
    Point Transform(Point src);
    void Update();

   private:
    float mu_ = 2;
    std::vector<Matrix22> transform_matrix_;
};