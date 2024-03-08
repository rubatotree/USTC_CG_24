#pragma once
#include <vector>
#include <tuple>
#include "geometry.h"

namespace USTC_CG
{
class ImageWarpingAlgorithm
{
   public:
    struct Sample
    {
        Point src, dest;
    };

    void AddSample(Sample sample)
    {
        samples.push_back(sample);
    };
    void AddSample(float srcx, float srcy, float destx, float desty)
    {
        samples.push_back({ Point(srcx, srcy), Point(destx, desty) });
    };
    virtual Point Transform(Point src)
    {
        return Point(0, 0);
    };
    virtual void Update(){};

   protected:
    std::vector<Sample> samples;
};
}  // namespace USTC_CG