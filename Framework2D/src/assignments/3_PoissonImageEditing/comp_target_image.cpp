#include "comp_target_image.h"
#include <Eigen/Dense>
#include <cmath>
#include <ctime>
#include <iostream>
#include "image_clone_algorithms/point_set.h"
#include "image_clone_algorithms/paste.h"
#include "image_clone_algorithms/seamless.h"
#include "image_clone_algorithms/mixed_gradients.h"

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

    PointSet* point_set = source_image_->get_selected_point_set();
    if (point_set == nullptr)
        return;
}

void CompTargetImage::clone()
{
    if (clone_type_ == USTC_CG::CompTargetImage::kDefault)
        return;
    PointI offset_tar = PointI(
        static_cast<int>(mouse_position_.x) -
            static_cast<int>(source_image_->get_position().x),
        static_cast<int>(mouse_position_.y) -
            static_cast<int>(source_image_->get_position().y));
    
    PointSet* point_set = source_image_->get_selected_point_set();
    restore();
    ImageCloneAlgorithm* cloner = nullptr;
    
    switch (clone_type_)
    {
        case USTC_CG::CompTargetImage::kPaste: 
            cloner = new ImageClonePaste(point_set, source_image_->get_data(), data_, offset_tar);
            break;
        case USTC_CG::CompTargetImage::kSeamless:
            cloner = new ImageCloneSeamless(
                point_set, source_image_->get_data(), data_, offset_tar);
            break;
        case USTC_CG::CompTargetImage::kMixedGradients:
            cloner = new ImageCloneMixedGradients(
                point_set, source_image_->get_data(), data_, offset_tar);
            break;
    }

    cloner->clone();

    delete cloner;

    update();
}
}  // namespace USTC_CG