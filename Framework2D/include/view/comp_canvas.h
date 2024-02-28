#pragma once

#include <imgui.h>

#include <memory>
#include <vector>
#include <stack>

#include "shapes/shape.h"
#include "view/component.h"

namespace USTC_CG
{

// Canvas class for drawing shapes.
class Canvas : public Component
{
   public:
    // Inherits constructor from Component.
    using Component::Component;

    // Override the draw method from the parent Component class.
    void draw() override;

    // Enumeration for supported shape types.
    enum ShapeType
    {
        kDefault = 0,
        kLine = 1,
        kRect = 2,
        kEllipse = 3,
        kPolygon = 4,
        kFreehand = 5,
        kEraser = 6
    };

    void update();

    // Shape type setters.
    void set_default();
    void set_line();
    void set_rect();
    void set_polygon();
    void set_ellipse();
    void set_freehand();
    void set_eraser();

    // 对象属性
    bool get_fill() { return isfill_; }
    float get_thickness() { return thickness_; }
    float* get_color() { return color_; }
    float get_smooth() { return smooth_; }
    void set_fill(bool isfill) { isfill_ = isfill; }
    void set_thickness(float thickness) { thickness_ = thickness; }
    void set_color(float col[3])
    {
        color_[0] = col[0];
        color_[1] = col[1];
        color_[2] = col[2];
    }
    void set_smooth(float smooth) { smooth_ = smooth; }

    ShapeType current_shape() { return shape_type_; }

    void undo_event();
    void redo_event();

    bool able_to_undo() { return !shape_list_.empty(); }
    bool able_to_redo() { return !redo_stack_.empty(); }

    // Clears all shapes from the canvas.
    void clear_shape_list();

    // Set canvas attributes (position and size).
    void set_attributes(const ImVec2& min, const ImVec2& size);

    // Controls the visibility of the canvas background.
    void show_background(bool flag);

   private:
    // Drawing functions.
    void draw_background();
    void draw_shapes();

    // Event handlers for mouse interactions.
    void mouse_click_event(ImGuiMouseButton button);
    void mouse_move_event();
    void mouse_release_event();

    float smooth_to_ratio(float smooth); // 从量化的平滑值到 lerp 混合比的映射

    // Calculates mouse's relative position in the canvas.
    ImVec2 mouse_pos_in_canvas() const;

    // Canvas attributes.
    ImVec2 canvas_min_;         // Top-left corner of the canvas.
    ImVec2 canvas_max_;         // Bottom-right corner of the canvas.
    ImVec2 canvas_size_;        // Size of the canvas.
    bool draw_status_ = false;  // Is the canvas currently being drawn on.

    ImVec2 canvas_minimal_size_ = ImVec2(50.f, 50.f);
    ImU32 background_color_ = IM_COL32(255, 255, 255, 255);
    ImU32 border_color_ = IM_COL32(50, 50, 50, 255);
    bool show_background_ = true;  // Controls background visibility.

    // Mouse interaction status.
    bool is_hovered_, is_active_;

    // Current shape being drawn.
    ShapeType shape_type_ = kDefault;
    ImVec2 start_point_, end_point_;
    std::shared_ptr<Shape> current_shape_;

    // List of shapes drawn on the canvas.
    std::vector<std::shared_ptr<Shape>> shape_list_;

    // 另开一个栈来存储重做操作
    std::stack<std::shared_ptr<Shape>> redo_stack_;

    bool isfill_ = false;
    float thickness_ = 2.0;
    float color_[3] = { 0, 0, 0 };

    // 量化的平滑值
    float smooth_ = 5.0;
    float smooth_x_ = 0.0, smooth_y_ = 0.0;
};

}  // namespace USTC_CG
