#include "comp_source_image.h"

#include <algorithm>
#include <cmath>
#include "view/shapes/line.h"
#include "view/shapes/rect.h"
#include "view/shapes/ellipse.h"
#include "view/shapes/polygon.h"
#include "view/shapes/freehand.h"

namespace USTC_CG
{
using uchar = unsigned char;

CompSourceImage::CompSourceImage(
    const std::string& label,
    const std::string& filename)
    : ImageEditor(label, filename)
{
    if (data_)
        selected_region_ =
            std::make_shared<Image>(data_->width(), data_->height(), 1);
}

void CompSourceImage::draw()
{
    // Draw the image
    ImageEditor::draw();

    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    bool is_hovered_ = ImGui::IsItemHovered();

    if (is_hovered_)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            mouse_click_event(ImGuiMouseButton_Left);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            mouse_click_event(ImGuiMouseButton_Right);
        mouse_move_event();
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            mouse_release_event();
    }
    
    if (current_shape_)
    {
        float bias[2] = { position_.x, position_.y };
        current_shape_->draw(bias);
    }
}

void CompSourceImage::mouse_click_event(ImGuiMouseButton button)
{
    if (flag_enable_selecting_region_ && !draw_status_)
    {
        draw_status_ = true;
        start_ = end_ = mouse_pos_in_image();
        switch (region_type_)
        {
            case USTC_CG::CompSourceImage::kDefault:
            {
                break;
            }
            case USTC_CG::CompSourceImage::kRect:
            {
                current_shape_ = std::make_shared<Rect>(
                    start_.x, start_.y, end_.x, end_.y);
                break;
            }
            case USTC_CG::CompSourceImage::kPolygon:
            {
                current_shape_ = std::make_shared<Polygon>(
                    start_.x, start_.y, end_.x, end_.y);
                break;
            }
            case USTC_CG::CompSourceImage::kFreehand:
            {
                current_shape_ = std::make_shared<Freehand>(
                    start_.x, start_.y, end_.x, end_.y);
                break;
            }
            default: break;
        }
        if (current_shape_)
        {
            float col[3] = { 1, 0, 0 };
            current_shape_->setConfig(Shape::Config(0, 1, col));
        }
    }
    else
    {
        if (current_shape_)
        {
            if (region_type_ == kPolygon)
            {
                auto current_polygon_ = ((Polygon*)(current_shape_.get()));
                // 多边形的左键功能与其他形状都不一样，特殊处理
                bool finished = false;
                if (button == ImGuiMouseButton_Left)
                {
                    // 左键：先检测是否被起点吸附，如果未被吸附则添加点，否则终止绘制
                    finished = current_polygon_->addPoint(mouse_pos_in_image().x, mouse_pos_in_image().y);
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
                    if (current_polygon_->size() <= 2)
                    {
                        current_shape_.reset();
                    }
                    else
                    {
                        finish_drawing();
                    }
                }
            }
            else
            {
                finish_drawing();
            }
        }
    }
}

void CompSourceImage::mouse_move_event()
{
    if (draw_status_)
    {
        end_ = mouse_pos_in_image();
        if (current_shape_)
        {
            current_shape_->update(end_.x, end_.y);
        }
    }
}

void CompSourceImage::mouse_release_event()
{
    if (draw_status_)
    {
        end_ = mouse_pos_in_image();
        if (region_type_ == kFreehand)
        {
            current_shape_->update(end_.x, end_.y);
            current_shape_->update(start_.x, start_.y); // Freehand 默认不连回起点
            finish_drawing();
        }
        if (region_type_ == kRect)
        {
            finish_drawing();
        }
    }
}

void CompSourceImage::finish_drawing()
{
    draw_status_ = false;
    delete selected_point_set_;

    std::vector<PointI> polygon;
    std::vector<std::pair<float, float>> pointlist;
    switch (region_type_)
    {
        case USTC_CG::CompSourceImage::kDefault: break;
        case USTC_CG::CompSourceImage::kRect:
        {
            polygon.push_back(PointI(start_.x, start_.y));
            polygon.push_back(PointI(end_.x, start_.y));
            polygon.push_back(PointI(end_.x, end_.y));
            polygon.push_back(PointI(start_.x, end_.y));
            break;
        }
        case USTC_CG::CompSourceImage::kPolygon:
        {
            pointlist =
                (dynamic_cast<Polygon*>(current_shape_.get()))->get_pointlist();
            for (auto vertex : pointlist)
            {
                polygon.push_back(
                    PointI((int)vertex.first, (int)vertex.second));
            }
            break;
        }
        case USTC_CG::CompSourceImage::kFreehand:
        {
            pointlist = (dynamic_cast<Freehand*>(current_shape_.get()))
                            ->get_pointlist();
            for (auto vertex : pointlist)
            {
                polygon.push_back(
                    PointI((int)vertex.first, (int)vertex.second));
            }
            break;
        }
        default: break;
    }
    selected_point_set_ = new PointSet(polygon);
}

ImVec2 CompSourceImage::mouse_pos_in_image() const
{
    // 这里还加入了限制鼠标不涂出边界。
    // 为了有有效的 Laplacian，边界上预留了一个像素。
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mouse_pos_in_image(
        io.MousePos.x - position_.x, io.MousePos.y - position_.y);
    if (mouse_pos_in_image.x < 1)
        mouse_pos_in_image.x = 1;
    else if (mouse_pos_in_image.x > data_->width() - 2)
        mouse_pos_in_image.x = data_->width() - 2;
    if (mouse_pos_in_image.y < 1)
        mouse_pos_in_image.y = 1;
    else if (mouse_pos_in_image.y > data_->height() - 2)
        mouse_pos_in_image.y = data_->height() - 2;

    return mouse_pos_in_image;
}

void CompSourceImage::enable_selecting(bool flag)
{
    flag_enable_selecting_region_ = flag;
}
std::shared_ptr<Image> CompSourceImage::get_region()
{
    return selected_region_;
}
std::shared_ptr<Image> CompSourceImage::get_data()
{
    return data_;
}
ImVec2 CompSourceImage::get_position() const
{
    return start_;
}
}  // namespace USTC_CG