#include "view/comp_canvas.h"

#include <cmath>
#include <iostream>

#include "imgui.h"
#include "view/shapes/line.h"
#include "view/shapes/rect.h"
#include "view/shapes/ellipse.h"
#include "view/shapes/polygon.h"
#include "view/shapes/freehand.h"

namespace USTC_CG
{
void Canvas::draw()
{
    draw_background();

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        mouse_click_event(ImGuiMouseButton_Left);
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        mouse_click_event(ImGuiMouseButton_Right);
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        mouse_release_event();

    // 撤销/重做快捷键
    if (ImGui::IsKeyChordPressed(ImGuiKey_ModCtrl | ImGuiKey_Z))
        undo_event();
    if (ImGui::IsKeyChordPressed(ImGuiKey_ModCtrl | ImGuiKey_Y))
        redo_event();
    if (ImGui::IsKeyChordPressed(ImGuiKey_ModCtrl | ImGuiKey_ModShift | ImGuiKey_Z))
        redo_event();


    draw_shapes();
}

void Canvas::set_attributes(const ImVec2& min, const ImVec2& size)
{
    canvas_min_ = min;
    canvas_size_ = size;
    canvas_minimal_size_ = size;
    canvas_max_ =
        ImVec2(canvas_min_.x + canvas_size_.x, canvas_min_.y + canvas_size_.y);
}

void Canvas::show_background(bool flag)
{
    show_background_ = flag;
}

void Canvas::set_default()
{
    draw_status_ = false;
    shape_type_ = kDefault;
}

void Canvas::set_line()
{
    draw_status_ = false;
    shape_type_ = kLine;
}

void Canvas::set_rect()
{
    draw_status_ = false;
    shape_type_ = kRect;
}

void Canvas::set_polygon()
{
    draw_status_ = false;
    shape_type_ = kPolygon;
}

void Canvas::set_eraser()
{
    draw_status_ = false;
    shape_type_ = kEraser;
}

void Canvas::set_ellipse()
{
    draw_status_ = false;
    shape_type_ = kEllipse;
}

void Canvas::set_freehand()
{
    draw_status_ = false;
    shape_type_ = kFreehand;
}

void Canvas::clear_shape_list()
{
    shape_list_.clear();
    while(!redo_stack_.empty()) redo_stack_.pop();
}

void Canvas::draw_background()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (show_background_)
    {
        // Draw background recrangle
        draw_list->AddRectFilled(canvas_min_, canvas_max_, background_color_);
        // Draw background border
        draw_list->AddRect(canvas_min_, canvas_max_, border_color_);
    }
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(canvas_min_);
    ImGui::InvisibleButton(
        label_.c_str(), canvas_size_, ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    is_hovered_ = ImGui::IsItemHovered();
    is_active_ = ImGui::IsItemActive();
}

void Canvas::draw_shapes()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float bias[2] = { canvas_min_.x, canvas_min_.y };
    // ClipRect can hide the drawing content outside of the rectangular area
    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(bias);
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(bias);
    }
    draw_list->PopClipRect();
}

void Canvas::mouse_click_event(ImGuiMouseButton button)
{
    if (shape_type_ == kDefault)
    {
        // 不预先设置为 kFreehand 是因为这样会导致菜单栏 Freehand 变黑，造成对用户的误导。
        // 感觉不是非常好的解决方式。
        shape_type_ = kFreehand;
    }
    if (!draw_status_)
    {
        draw_status_ = true;
        start_point_ = end_point_ = mouse_pos_in_canvas();
        switch (shape_type_)
        {
            case USTC_CG::Canvas::kDefault:
            {
                break;
            }
            case USTC_CG::Canvas::kLine:
            {
                current_shape_ = std::make_shared<Line>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kRect:
            {
                current_shape_ = std::make_shared<Rect>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kEllipse:
            {
                current_shape_ = std::make_shared<Ellipse>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kPolygon:
            {
                current_shape_ = std::make_shared<Polygon>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kFreehand:
            case USTC_CG::Canvas::kEraser:
            {
                current_shape_ = std::make_shared<Freehand>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            default: break;
        }

        if (current_shape_ != nullptr)
        {
            float col_white[3] = { 1.0f, 1.0f, 1.0f };
            float* col = col_white;

            if (shape_type_ != kEraser)
                col = color_;

            current_shape_->setConfig(
                Shape::Config(isfill_, thickness_, col));
        }

        // 设置平滑
        smooth_x_ = start_point_.x;
        smooth_y_ = start_point_.y;
    }
    else
    {
        if (current_shape_)
        {
            if (shape_type_ == kPolygon)
            {
                auto current_polygon_ = ((Polygon*)(current_shape_.get()));
                // 多边形的左键功能与其他形状都不一样，特殊处理
                bool finished = false;
                if (button == ImGuiMouseButton_Left)
                {
                    // 左键：先检测是否被起点吸附，如果未被吸附则添加点，否则终止绘制
                    finished = current_polygon_->addPoint(
                                       mouse_pos_in_canvas().x,
                                       mouse_pos_in_canvas().y);
                }
                if (button == ImGuiMouseButton_Right)
                {
                    // 右键：将当前点连接到起点上，直接终止绘制
                    current_polygon_->finishDrawing();
                    finished = true;
                }
                
                if (finished)
                {
                    // 如果多边形仅有一个点，则没必要绘制
                    if (current_polygon_->size() >= 2)
                    {
                        shape_list_.push_back(current_shape_);
                        while(!redo_stack_.empty()) redo_stack_.pop();
                    }
                    current_shape_.reset();
                    draw_status_ = false;
                    std::cout << "Polygon Finish" << std::endl;
                }
            }
            else
            {
                shape_list_.push_back(current_shape_);
                // 一旦做出操作，就清空重做栈
                while (!redo_stack_.empty()) redo_stack_.pop();
                current_shape_.reset();
                draw_status_ = false;
            }
        }
    }
}

void Canvas::mouse_move_event()
{
    if (draw_status_)
    {
        end_point_ = mouse_pos_in_canvas();
        if (shape_type_ == kFreehand || shape_type_ == kEraser)
        {
            end_point_.x = smooth_x_;
            end_point_.y = smooth_y_;
        }
        if (current_shape_)
        {
            current_shape_->update(end_point_.x, end_point_.y);
        }
    }
}

void Canvas::mouse_release_event()
{
    if (draw_status_)
    {
        end_point_ = mouse_pos_in_canvas();
        if (shape_type_ == kFreehand || shape_type_ == kEraser)
        {
            // 实际上在平滑下应该先等待画笔移动到终点再停止。这里直接停止了。
            // 如果要等待的话则等待时间内无法画下一笔，这个在现有的框架内感觉有点难以实现。
            current_shape_->update(smooth_x_, smooth_y_);
            shape_list_.push_back(current_shape_);
            while(!redo_stack_.empty()) redo_stack_.pop();
            current_shape_.reset();
            draw_status_ = false;
        }
    }
}

void Canvas::undo_event()
{
    if (draw_status_)
    {
        // 正在绘制，则取消绘制
        current_shape_.reset();
        draw_status_ = false;
    }
    else
    {
        // 否则弹出栈顶
        if (shape_list_.size() > 0)
        {
            redo_stack_.push(shape_list_.back());
            shape_list_.pop_back();
        }
    }
}

void Canvas::redo_event()
{
    if (!redo_stack_.empty())
    {
        shape_list_.push_back(redo_stack_.top());
        redo_stack_.pop();
    }
}

float Canvas::smooth_to_ratio(float smooth)
{
    return pow(0.775, smooth);
}

void Canvas::update()
{
    // 利用线性插值方法做平滑
    // 相当于把游戏开发中平滑移动的方式搬到这里了
    auto current_point = mouse_pos_in_canvas();
    float m = smooth_to_ratio(smooth_);
    smooth_x_ = m * current_point.x + (1 - m) * smooth_x_;
    smooth_y_ = m * current_point.y + (1 - m) * smooth_y_;
}

ImVec2 Canvas::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse_pos_in_canvas(
        io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
    return mouse_pos_in_canvas;
}
}  // namespace USTC_CG