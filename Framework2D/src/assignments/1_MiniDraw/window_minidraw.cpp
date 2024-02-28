#include "window_minidraw.h"

#include <iostream>

namespace USTC_CG
{
MiniDraw::MiniDraw(const std::string& window_name) : Window(window_name)
{
    p_canvas_ = std::make_shared<Canvas>("Cmpt.Canvas");
}

MiniDraw::~MiniDraw()
{
}

void MiniDraw::draw()
{
    draw_canvas();
}

void MiniDraw::draw_canvas()
{
    // Set a full screen canvas view
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(96, 96, 96, 255));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 40, 255));
    if (ImGui::Begin(
            "Canvas",
            &flag_show_canvas_view_,
            ImGuiWindowFlags_NoDecoration /* | ImGuiWindowFlags_NoBackground*/))
    {
        // Çå¿Õ»­²¼
        if (ImGui::Button("New"))
        {
            std::cout << "New" << std::endl;
            p_canvas_->clear_shape_list();
        }
        ImGui::SameLine();

        bool disable_undo = !p_canvas_->able_to_undo();
        if (disable_undo) ImGui::BeginDisabled();
        if (ImGui::Button("Undo"))
        {
            std::cout << "Undo" << std::endl;
            p_canvas_->undo_event();
        }
        if (disable_undo) ImGui::EndDisabled();
        ImGui::SameLine();

        bool disable_redo = !p_canvas_->able_to_redo();
        if (disable_redo) ImGui::BeginDisabled();
        if (ImGui::Button("Redo"))
        {
            std::cout << "Redo" << std::endl;
            p_canvas_->redo_event();
        }
        if (disable_redo) ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        auto current_shape = p_canvas_->current_shape();
        if (current_shape == Canvas::kLine) ImGui::BeginDisabled();
        if (ImGui::Button("Line"))
        {
            std::cout << "Set shape to Line" << std::endl;
            p_canvas_->set_line();
        }
        if (current_shape == Canvas::kLine) ImGui::EndDisabled();
        ImGui::SameLine();
        
        if (current_shape == Canvas::kRect) ImGui::BeginDisabled();
        if (ImGui::Button("Rect"))
        {
            std::cout << "Set shape to Rect" << std::endl;
            p_canvas_->set_rect();
        }
        if (current_shape == Canvas::kRect) ImGui::EndDisabled();
        ImGui::SameLine();

        if (current_shape == Canvas::kEllipse) ImGui::BeginDisabled();
        if (ImGui::Button("Ellipse"))
        {
            std::cout << "Set shape to Ellipse" << std::endl;
            p_canvas_->set_ellipse();
        }
        if (current_shape == Canvas::kEllipse) ImGui::EndDisabled();
        ImGui::SameLine();

        if (current_shape == Canvas::kPolygon) ImGui::BeginDisabled();
        if (ImGui::Button("Polygon"))
        {
            std::cout << "Set shape to Polygon" << std::endl;
            p_canvas_->set_polygon();
        }
        if (current_shape == Canvas::kPolygon) ImGui::EndDisabled();
        ImGui::SameLine();

        if (current_shape == Canvas::kFreehand) ImGui::BeginDisabled();
        if (ImGui::Button("Freehand"))
        {
            std::cout << "Set shape to Freehand" << std::endl;
            p_canvas_->set_freehand();
        }
        if (current_shape == Canvas::kFreehand) ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        if (current_shape == Canvas::kEraser) ImGui::BeginDisabled();
        if (ImGui::Button("Eraser"))
        {
            std::cout << "Set tool to Eraser" << std::endl;
            p_canvas_->set_eraser();
        }
        if (current_shape == Canvas::kEraser) ImGui::EndDisabled();
        
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Fill Ñ¡¿ò
        bool fill = p_canvas_->get_fill();
        ImGui::Checkbox("Fill", &fill);
        p_canvas_->set_fill(fill);
        ImGui::SameLine();
        
        // ÑÕÉ«¿ò
        float* col = p_canvas_->get_color();
        ImGui::ColorEdit3(
            "Color", col, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoInputs);
        p_canvas_->set_color(col);
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // »­±Ê¿í¶È
        ImGui::SetNextItemWidth(120);
        float thickness = p_canvas_->get_thickness();
        ImGui::SliderFloat("Thickness", &thickness, 1.0f, 20.0f, "%.1f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        p_canvas_->set_thickness(thickness);
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // »­±ÊÆ½»¬
        ImGui::SetNextItemWidth(120);
        float smooth = p_canvas_->get_smooth();
        ImGui::SliderFloat("Smooth", &smooth, 0.0f, 10.0f, "%.1f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        p_canvas_->set_smooth(smooth);

        // Canvas component
        ImGui::Text("Press left mouse to add shapes.");

        // Set the canvas to fill the rest of the window
        const auto& canvas_min = ImGui::GetCursorScreenPos();
        const auto& canvas_size = ImGui::GetContentRegionAvail();
        p_canvas_->set_attributes(canvas_min, canvas_size);
        p_canvas_->update();
        p_canvas_->draw();
    }

    ImGui::PopStyleColor(2);
    ImGui::End();
}
}  // namespace USTC_CG