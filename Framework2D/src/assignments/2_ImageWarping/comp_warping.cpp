#include "comp_warping.h"
#include "annoylib.h"
#include "kissrandom.h"
#include <cmath>
#include <iostream>
#include <ctime>
using namespace Annoy;

namespace USTC_CG
{
using uchar = unsigned char;

CompWarping::CompWarping(const std::string& label, const std::string& filename)
    : ImageEditor(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void CompWarping::draw()
{
    // Draw the image
    ImageEditor::draw();
    // Draw the canvas
    if (flag_enable_selecting_points_)
        select_points();
}
void CompWarping::draw_control_points()
{
    if (!control_point_visible_)
        return;
    auto draw_list = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < start_points_.size(); ++i)
    {
        ImVec2 s(
            start_points_[i].x + position_.x, start_points_[i].y + position_.y);
        ImVec2 e(
            end_points_[i].x + position_.x, end_points_[i].y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }
    if (draw_status_)
    {
        ImVec2 s(start_.x + position_.x, start_.y + position_.y);
        ImVec2 e(end_.x + position_.x, end_.y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
    }
}

void CompWarping::invert()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            data_->set_pixel(
                i,
                j,
                { static_cast<uchar>(255 - color[0]),
                  static_cast<uchar>(255 - color[1]),
                  static_cast<uchar>(255 - color[2]) });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void CompWarping::mirror(bool is_horizontal, bool is_vertical)
{
    Image image_tmp(*data_);
    int width = data_->width();
    int height = data_->height();

    if (is_horizontal)
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i,
                        j,
                        image_tmp.get_pixel(width - 1 - i, height - 1 - j));
                }
            }
        }
        else
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(width - 1 - i, j));
                }
            }
        }
    }
    else
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(i, height - 1 - j));
                }
            }
        }
    }

    // After change the image, we should reload the image data to the renderer
    update();
}
void CompWarping::gray_scale()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            uchar gray_value = (color[0] + color[1] + color[2]) / 3;
            data_->set_pixel(i, j, { gray_value, gray_value, gray_value });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void CompWarping::warping()
{
    int size = start_points_.size(), width = data_->width(), height = data_->height();
    // Ԥ����ͼ��
    Image warped_image(*data_);
    
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            warped_image.set_pixel(x, y, { 0, 0, 0 });
        }
    }
    
    // ��ʼ�� warper
    ImageWarpingAlgorithm* warper = nullptr;

    if (image_warping_algorithm_type_ == Warping_IDW)
    {
        warper = new WarpingIDW(idw_mu_);
        std::cout << "Algorithm: IDW" << std::endl;
    }
    else if (image_warping_algorithm_type_ == Warping_RBF)
    {
        warper = new WarpingRBF(rbf_r_, rbf_mu_);
        std::cout << "Algorithm: RBF" << std::endl;
    }
    else if (image_warping_algorithm_type_ == Warping_Fisheye)
    {
        warper = new WarpingFisheye(width, height);
        std::cout << "Algorithm: Fisheye" << std::endl;
    }

    assert(warper != nullptr);

    bool ann = fill_hole_algorithm_type_ == FillHole_ANN;
    bool reverse = fill_hole_algorithm_type_ == FillHole_Reverse;

    if (ann)
        std::cout << "Fill Hole Method: ANN (" << ann_sample_n_ << " Sample(s))"
                  << std::endl;
    else if (reverse)
        std::cout << "Fill Hole Method: Reverse" << std::endl;
    else 
        std::cout << "Fill Hole Method: None" << std::endl;

    // ��ʼ�� ANN
    AnnoyIndex<
        int,
        float,
        Euclidean,
        Kiss32Random,
        AnnoyIndexSingleThreadedBuildPolicy>
        index(2);
    vector<bool> visited;   // ������ص��Ƿ���ɫ
    visited.resize(width * height, false);
    
    // ���������� warper
    for (int i = 0; i < size; i++)
    {
        if (!reverse)
        {
            warper->AddSample(
                start_points_[i].x,
                start_points_[i].y,
                end_points_[i].x,
                end_points_[i].y);
        }
        else
        {
            warper->AddSample(
                end_points_[i].x,
                end_points_[i].y,
                start_points_[i].x,
                start_points_[i].y);
        }
    }
    std::cout << "Number of Control Points: " << size << std::endl;
    std::cout << "Image Size:  " << width << " * " << height << std::endl;

    // ��ʱ���
    clock_t timer = clock();
    warper->Update();
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            Point new_pos = warper->Transform(Point(x, y));
            int new_x = static_cast<int>(new_pos.x);
            int new_y = static_cast<int>(new_pos.y);
            if (new_x >= 0 && new_x < width && new_y >= 0 &&
                new_y < height)
            {
                if (!reverse)
                {
                    // None
                    std::vector<unsigned char> pixel = data_->get_pixel(x, y);
                    warped_image.set_pixel(new_x, new_y, pixel);
                    if (ann)
                    {
                        // ANN
                        std::vector<float> vec;
                        vec.resize(2, 0);
                        vec[0] = (float)new_x;
                        vec[1] = (float)new_y;
                        index.add_item(width * new_y + new_x, vec.data());
                        visited[width * new_y + new_x] = true;
                    }
                }
                else
                {
                    // Reverse
                    std::vector<unsigned char> pixel = data_->get_pixel(new_x, new_y);
                    warped_image.set_pixel(x, y, pixel);
                }
            }
        }
    }
    if (ann)
    {
        // ANN
        index.build(ann_sample_n_);
        
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (!visited[y * width + x])
                {
                    std::vector<int> closest_items;
                    std::vector<float> distances;
                    std::vector<float> vec;
                    vec.resize(2, 0);
                    vec[0] = (float)x;
                    vec[1] = (float)y;
                    index.get_nns_by_vector(
                        vec.data(),
                        ann_sample_n_,
                        -1,
                        &closest_items,
                        &distances);
                    float r = 0, g = 0, b = 0;
                    for (int i = 0; i < closest_items.size(); ++i)
                    {
                        auto pixel = warped_image.get_pixel(closest_items[i] % width, closest_items[i] / width);
                        r += pixel[0];
                        g += pixel[1];
                        b += pixel[2];
                    }
                    std::vector<unsigned char> pixel;
                    pixel.resize(3, 0);
                    pixel[0] = static_cast<unsigned char>(r / closest_items.size());
                    pixel[1] = static_cast<unsigned char>(g / closest_items.size());
                    pixel[2] = static_cast<unsigned char>(b / closest_items.size());
                    warped_image.set_pixel(x, y, pixel);
                }
            }
        }
    }
    std::cout << "Time: " << ((double)(clock() - timer)) / CLK_TCK << "s" << std::endl;
    std::cout << std::endl;
    *data_ = std::move(warped_image);
    update();

    delete warper;
}
void CompWarping::restore()
{
    *data_ = *back_up_;
    update();
}
void CompWarping::enable_selecting(bool flag)
{
    flag_enable_selecting_points_ = flag;
}
void CompWarping::select_points()
{
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    bool is_hovered_ = ImGui::IsItemHovered();
    // Selections
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        start_ = end_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
    }
    if (draw_status_)
    {
        end_ = ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            start_points_.push_back(start_);
            end_points_.push_back(end_);
            draw_status_ = false;
        }
    }
    draw_control_points();
}
void CompWarping::init_selections()
{
    start_points_.clear();
    end_points_.clear();
}
}  // namespace USTC_CG