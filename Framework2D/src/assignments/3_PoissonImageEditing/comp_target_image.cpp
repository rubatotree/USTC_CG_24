#include "comp_target_image.h"
#include <Eigen/Dense>
#include <cmath>
#include <ctime>
#include <iostream>
namespace USTC_CG
{
using uchar = unsigned char;

CompTargetImage::CompTargetImage(
    const std::string& label,
    const std::string& filename)
    : ImageEditor(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void CompTargetImage::draw()
{
    // Draw the image
    ImageEditor::draw();
    // Invisible button for interactions
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    bool is_hovered_ = ImGui::IsItemHovered();
    // When the mouse is clicked or moving, we would adapt clone function to
    // copy the selected region to the target.
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        edit_status_ = true;
        mouse_position_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        clone();
    }
    if (edit_status_)
    {
        mouse_position_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        if (flag_realtime_updating)
            clone();
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            edit_status_ = false;
        }
    }
}

void CompTargetImage::set_source(std::shared_ptr<CompSourceImage> source)
{
    source_image_ = source;
}

void CompTargetImage::set_realtime(bool flag)
{
    flag_realtime_updating = flag;
}

void CompTargetImage::restore()
{
    *data_ = *back_up_;
    update();
}
void CompTargetImage::clone()
{
    if (clone_type_ == USTC_CG::CompTargetImage::kDefault)
        return;

    std::shared_ptr<Image> mask = source_image_->get_region();
    PointSet* point_set = source_image_->get_selected_point_set();
    restore();

    PointI offset_tar = PointI(
        static_cast<int>(mouse_position_.x) -
            static_cast<int>(source_image_->get_position().x),
        static_cast<int>(mouse_position_.y) -
            static_cast<int>(source_image_->get_position().y));
    
    int size = point_set->size();
    Eigen::VectorXf F[3] = { Eigen::VectorXf(size),
                             Eigen::VectorXf(size),
                             Eigen::VectorXf(size) },
                    G[3] = { Eigen::VectorXf(size),
                             Eigen::VectorXf(size),
                             Eigen::VectorXf(size) };

    for (int i = 0; i < size; i++)
    {
        PointI psrc = point_set->get_point_at(i);
        PointI ptar = psrc + offset_tar;
        PointSet::PointType type = point_set->check(psrc);

        switch (clone_type_)
        {
            case USTC_CG::CompTargetImage::kPaste:
                if (ptar.x < 0 || ptar.x >= image_width_ || ptar.y < 0 ||
                    ptar.y >= image_height_)
                    continue;
                data_->set_pixel(
                    ptar.x,
                    ptar.y,
                    source_image_->get_data()->get_pixel(psrc.x, psrc.y));
                break;

            case USTC_CG::CompTargetImage::kSeamless:
                if (type == PointSet::kBoundary)
                {
                    // Dirichlet 边界条件
                    auto col = col_tar(ptar.x, ptar.y, 0);
                    for (int c = 0; c < 3; c++)
                        F[c](i) = col[c];
                }
                else if (type == PointSet::kIn)
                {
                    // Laplacian
                    auto col = col_src(psrc.x, psrc.y, 0);

                    for (int c = 0; c < 3; c++)
                        col[c] *= -4;
                    for (int d = 1; d <= 4; d++)
                    {
                        auto col_d = col_src(psrc.x, psrc.y, d);
                        for (int c = 0; c < 3; c++) col[c] += col_d[c];
                    }

                    for (int c = 0; c < 3; c++)
                        F[c](i) = col[c];
                }
                break;

            case USTC_CG::CompTargetImage::kMixedGradients:
                if (type == PointSet::kBoundary)
                {
                    auto col = col_tar(ptar.x, ptar.y, 0);
                    for (int c = 0; c < 3; c++)
                        F[c](i) = col[c];
                }
                else if (type == PointSet::kIn)
                {
                    std::vector<float> ctar[5], csrc[5];
                    for (int d = 0; d <= 4; d++)
                    {
                        ctar[d] = col_tar(ptar.x, ptar.y, d);
                        csrc[d] = col_src(psrc.x, psrc.y, d);
                    }
                    for (int c = 0; c < 3; c++)
                    {
                        float laplacian = 0;
                        for (int d = 1; d <= 4; d++)
                        {
                            float gradf = ctar[d][c] - ctar[0][c];
                            float gradg = csrc[d][c] - csrc[0][c];

                            // 始终取最大梯度
                            if (abs(gradf) > abs(gradg))
                                laplacian += gradf;
                            else
                                laplacian += gradg;
                        }
                        F[c](i) = laplacian;
                    }
                }
                break;
        }
    }
    if (clone_type_ == CompTargetImage::kSeamless ||
        clone_type_ == CompTargetImage::kMixedGradients)
    {
        for (int c = 0; c < 3; c++)
            G[c] = point_set->solve(F[c]);

        for (int i = 0; i < size; i++)
        {
            PointI ptar = point_set->get_point_at(i) + offset_tar;
            if (ptar.x < 0 || ptar.x >= image_width_ || ptar.y < 0 ||
                ptar.y >= image_height_)
                continue;

            // 防止颜色值超界
            uchar r = std::clamp(G[0](i), (float)0, (float)255);
            uchar g = std::clamp(G[1](i), (float)0, (float)255);
            uchar b = std::clamp(G[2](i), (float)0, (float)255);
            data_->set_pixel(ptar.x, ptar.y, {r, g, b});
        }
    }

    update();
}


// 这两个稍加封装的函数同时处理了三个问题：
// 1. 将颜色值转为 float， 便于计算；
// 2. 判断超界，并设置超界范围的颜色；
// 3. 提供获取邻域颜色的功能，简化代码。dir 取 1 到 4 分别对应右、左、上、下。

std::vector<float> CompTargetImage::col_src(int x, int y, int dir = 0)
{
    x += DIRMAPX[dir];
    y += DIRMAPY[dir];
    if (x < 0)
        x = 0;
    else if (x >= source_image_->get_data()->width())
        x = source_image_->get_data()->width() - 1;
    if (y < 0)
        y = 0;
    else if (y >= source_image_->get_data()->height())
        y = source_image_->get_data()->height() - 1;
    std::vector<float> ret;
    auto pixel = source_image_->get_data()->get_pixel(x, y);
    ret.resize(3, 0);
    for (int c = 0; c < 3; c++)
        ret[c] = (float)pixel[c];
    return ret;
}

std::vector<float> CompTargetImage::col_tar(int x, int y, int dir = 0)
{
    x += DIRMAPX[dir];
    y += DIRMAPY[dir];
    if (x < 0)
        x = 0;
    else if (x >= data_->width())
        x = data_->width() - 1;
    if (y < 0)
        y = 0;
    else if (y >= data_->height())
        y = data_->height() - 1;
    std::vector<float> ret;
    auto pixel = data_->get_pixel(x, y);
    ret.resize(3, 0);
    for (int c = 0; c < 3; c++)
        ret[c] = (float)pixel[c];
    return ret;
}

}  // namespace USTC_CG