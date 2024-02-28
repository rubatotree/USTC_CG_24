#include "view/shapes/rect.h"

#include <imgui.h>

namespace USTC_CG
{
// Draw the rectangle using ImGui
void Rect::draw(float bias[2]) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (config_.fill)
    {
        draw_list->AddRectFilled(
            ImVec2(bias[0] + start_point_x_, bias[1] + start_point_y_),
            ImVec2(bias[0] + end_point_x_, bias[1] + end_point_y_),
            IM_COL32(
                config_.line_color[0],
                config_.line_color[1],
                config_.line_color[2],
                config_.line_color[3]),
            0.f,  // No rounding of corners
            ImDrawFlags_None);
    }
    else
    {
        draw_list->AddRect(
            ImVec2(bias[0] + start_point_x_, bias[1] + start_point_y_),
            ImVec2(bias[0] + end_point_x_, bias[1] + end_point_y_),
            IM_COL32(
                config_.line_color[0],
                config_.line_color[1],
                config_.line_color[2],
                config_.line_color[3]),
            0.f,  // No rounding of corners
            ImDrawFlags_None,
            config_.line_thickness);
    }
}

void Rect::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

}  // namespace USTC_CG
