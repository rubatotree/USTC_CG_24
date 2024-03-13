#pragma once

#include "comp_source_image.h"
#include "view/comp_image.h"

namespace USTC_CG
{
class CompTargetImage : public ImageEditor
{
   public:
    enum CloneType
    {
        kDefault = 0,
        kPaste = 1,
        kSeamless = 2,
        kMixedGradients = 3
    };

    explicit CompTargetImage(
        const std::string& label,
        const std::string& filename);
    virtual ~CompTargetImage() noexcept = default;

    void draw() override;
    // Bind the source image component
    void set_source(std::shared_ptr<CompSourceImage> source);
    // Enable realtime updating
    void set_realtime(bool flag);
    void restore();

    CloneType get_clone_type()
    {
        return clone_type_;
    }
    void set_clone_type(CloneType type)
    {
        clone_type_ = type;
    }
    // The clone function
    void clone();

   private:
    // Store the original image data
    std::shared_ptr<Image> back_up_;
    // Source image
    std::shared_ptr<CompSourceImage> source_image_;
    CloneType clone_type_ = kDefault;

    ImVec2 mouse_position_;
    bool edit_status_ = false;
    bool flag_realtime_updating = true;
};

}  // namespace USTC_CG