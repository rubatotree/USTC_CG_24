#pragma once

#include "view/comp_image.h"
#include "image_clone_algorithms/point_set.h"
#include "view/shapes/shape.h"

namespace USTC_CG
{
class CompSourceImage : public ImageEditor
{
   public:
    // HW3_TODO(optional): Add more region shapes like polygon and freehand.
    enum RegionType
    {
        kDefault = 0,
        kRect = 1,
        kPolygon = 2,
        kFreehand = 3
    };

    explicit CompSourceImage(
        const std::string& label,
        const std::string& filename);
    virtual ~CompSourceImage();

    void draw() override;

    // Point selecting interaction
    void enable_selecting(bool flag);
    // Get the selected region in the source image, this would be a binary mask
    std::shared_ptr<Image> get_region();
    // Get the image data
    std::shared_ptr<Image> get_data();
    // Get the position to locate the region in the target image
    ImVec2 get_position() const;
    PointSet* get_selected_point_set() { return selected_point_set_; }
    RegionType get_region_type()
    {
        return region_type_;
    }

    void set_region_type(RegionType type)
    {
        region_type_ = type;
    }

    void mouse_click_event(ImGuiMouseButton button);
    void mouse_move_event();
    void mouse_release_event();

   private:
    RegionType region_type_ = kFreehand;
    std::shared_ptr<Shape> current_shape_;
    std::shared_ptr<Image> selected_region_;
    PointSet* selected_point_set_ = nullptr;
    ImVec2 start_, end_;
    bool flag_enable_selecting_region_ = true;
    bool draw_status_ = false;

    ImVec2 mouse_pos_in_image() const;
    void finish_drawing();
};

}  // namespace USTC_CG