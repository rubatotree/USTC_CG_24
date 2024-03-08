#include "view/shapes/freehand.h"

#include <imgui.h>

namespace USTC_CG
{
Freehand::Freehand(
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

void Freehand::draw(float bias[2]) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto color = IM_COL32(config_.line_color[0], config_.line_color[1], config_.line_color[2], config_.line_color[3]);
    
    for (int i = 0; i < pointlist.size() - 1; i++)
    {
        draw_list->AddLine(
            ImVec2(bias[0] + pointlist[i].first, bias[1] + pointlist[i].second),
            ImVec2(bias[0] + pointlist[i + 1].first, bias[1] + pointlist[i + 1].second),
            color,
            config_.line_thickness);
    }
    // 在节点处画圆，防止裂缝
    for (int i = 0; i < pointlist.size(); i++)
    {
        draw_list->AddCircleFilled(ImVec2(
            bias[0] + pointlist[i].first,
            bias[1] + pointlist[i].second),
            config_.line_thickness / 2 - 0.5,
            color,
            16
        );
    }
}

void Freehand::addPoint(float x, float y)
{
    pointlist.push_back({ x, y });
}

void Freehand::update(float x, float y)
{
    addPoint(x, y);
}
}  // namespace USTC_CG