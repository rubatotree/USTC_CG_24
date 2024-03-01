#include "view/shapes/polygon.h"

#include <imgui.h>

namespace USTC_CG
{
Polygon::Polygon(
    float start_point_x,
    float start_point_y,
    float end_point_x,
    float end_point_y)
    : start_point_x_(start_point_x),
      start_point_y_(start_point_y),
      end_point_x_(end_point_x),
      end_point_y_(end_point_y)
{
    addPoint(start_point_x, start_point_y);
}

void Polygon::draw(float bias[2]) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto color = IM_COL32(config_.line_color[0], config_.line_color[1], config_.line_color[2], config_.line_color[3]);
    
    int size = pointlist.size() + 1;
    auto points = new ImVec2[size];
    for (int i = 0; i < size - 1; i++)
    {
        points[i] = ImVec2(bias[0] + pointlist[i].first, bias[1] + pointlist[i].second);
    }
    // end_point 不会存入pointlist中，所以要单独绘制
    points[size - 1] = ImVec2(bias[0] + end_point_x_, bias[1] + end_point_y_);

    if (finished && config_.fill)
    {
        // 不支持非简单多边形
        draw_list->AddConvexPolyFilled(points, size, color);
    }
    else
    {
        // 锐角处线宽会变窄 不知道有没有自带的解决方法
        draw_list->AddPolyline(
            points,
            size,
            color,
            ImDrawFlags_::ImDrawFlags_RoundCornersAll,
            config_.line_thickness);
    }

    delete[] points;
}

// 返回添加该点后多边形绘制是否结束
bool Polygon::addPoint(float x, float y)
{
    if (pointlist.size() >= 2 && inFinishArea(x, y))
    {
        finishDrawing();
        return true;
    }
    pointlist.push_back({ x, y });
    return false;
}

// 终止多边形绘制
void Polygon::finishDrawing()
{
    end_point_x_ = start_point_x_;
    end_point_y_ = start_point_y_;
    finished = true;
}

// 判断一个点是否会被起点吸附
bool Polygon::inFinishArea(float x, float y)
{
    return (x - start_point_x_) * (x - start_point_x_) +
           (y - start_point_y_) * (y - start_point_y_) <
           FinishAreaRadius * FinishAreaRadius;
}

void Polygon::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
    if (pointlist.size() >= 2 && inFinishArea(end_point_x_, end_point_y_))
    {
        // 当点数超过一个时，在接近起始点处吸附
        end_point_x_ = start_point_x_;
        end_point_y_ = start_point_y_;
    }
}

int Polygon::size()
{
    return pointlist.size();
}

}  // namespace USTC_CG
