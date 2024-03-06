#include "image_warping_algorithms/fisheye.h"

Point WarpingFisheye::Transform(Point src)
{
    float center_x = width_ / 2.0f;
    float center_y = height_ / 2.0f;
    float dx = src.x - center_x;
    float dy = src.y - center_y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Simple non-linear transformation r -> r' = f(r)
    float new_distance = std::sqrt(distance) * 10;

    if (distance == 0)
    {
        return Point(center_x, center_y);
    }
    float ratio = new_distance / distance;
    float new_x = center_x + dx * ratio;
    float new_y = center_y + dy * ratio;

    return Point(new_x, new_y);
}

void WarpingFisheye::Update()
{
}
