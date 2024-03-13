#pragma once

#include <tuple>
#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Freehand : public Shape
{
   public:
    Freehand() = default;

    // Constructor to initialize a line with start and end coordinates
    Freehand(
        float start_point_x,
        float start_point_y,
        float end_point_x,
        float end_point_y);

    virtual ~Freehand() = default;

    // Overrides draw function to implement line-specific drawing logic
    void draw(float bias[2]) const override;

    // Overrides Shape's update function to adjust the end point during
    // interaction
    void update(float x, float y) override;
    void addPoint(float x, float y);

    std::vector<std::pair<float, float>>& get_pointlist()
    {
        return pointlist;
    }

   private:
    float start_point_x_, start_point_y_, end_point_x_, end_point_y_;
    std::vector<std::pair<float, float>> pointlist;
};
}  // namespace USTC_CG
