#include "view/shapes/ellipse.h"
#include <cmath>
#include <imgui.h>

namespace USTC_CG
{
void Ellipse::draw(float bias[2]) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 vLU = ImVec2(bias[0] + start_point_x_, bias[1] + start_point_y_),
           vRD = ImVec2(bias[0] + end_point_x_, bias[1] + end_point_y_);
    ImVec2 vCenter = ImVec2((vLU.x + vRD.x) / 2, (vLU.y + vRD.y) / 2);
    ImVec2 vSize = ImVec2(abs(vLU.x - vRD.x) / 2, abs(vLU.y - vRD.y) / 2);
    auto color = IM_COL32(config_.line_color[0],
                          config_.line_color[1],
                          config_.line_color[2],
                          config_.line_color[3]);
    if (config_.fill)
        draw_list->AddEllipseFilled(vCenter, vSize.x, vSize.y, color, 0.f, 0.f);
    else
        draw_list->AddEllipse(vCenter, vSize.x, vSize.y, color, 0.f, 0.f, config_.line_thickness);
}

void Ellipse::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

}  // namespace USTC_CG
