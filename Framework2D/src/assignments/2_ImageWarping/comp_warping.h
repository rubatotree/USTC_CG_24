#pragma once

#include "view/comp_image.h"
#include "image_warping_algorithms/image_warping.h"
#include "image_warping_algorithms/idw.h"
#include "image_warping_algorithms/rbf.h"
#include "image_warping_algorithms/fisheye.h"

namespace USTC_CG
{
// Image component for warping and other functions
class CompWarping : public ImageEditor
{
   public:

    enum ImageWarpingAlgorithmType
    {
        Warping_IDW,
        Warping_RBF,
        Warping_Fisheye
    };
    enum FillHoleAlgorithmType
    {
        FillHole_None,
        FillHole_ANN,
        FillHole_Reverse
    };
    explicit CompWarping(const std::string& label, const std::string& filename);
    virtual ~CompWarping() noexcept = default;

    void draw() override;
    void draw_control_points();

    // Simple edit functions
    void invert();
    void mirror(bool is_horizontal, bool is_vertical);
    void gray_scale();
    void warping();
    void restore();

    // Point selecting interaction
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();

    ImageWarpingAlgorithmType get_image_warping_algorithm()
    {
        return image_warping_algorithm_type_;
    }
    void set_image_warping_algorithm(
        ImageWarpingAlgorithmType image_warping_algorithm_type)
    {
        image_warping_algorithm_type_ = image_warping_algorithm_type;
    }
    FillHoleAlgorithmType get_fill_hole_algorithm()
    {
        return fill_hole_algorithm_type_;
    }
    void set_fill_hole_algorithm(FillHoleAlgorithmType fill_hole_algorithm_type)
    {
        fill_hole_algorithm_type_ = fill_hole_algorithm_type;
    }
    float get_idw_mu()
    {
        return idw_mu_;    
    }
    float get_rbf_r()
    {
        return rbf_r_;
    }
    float get_rbf_mu()
    {
        return rbf_mu_;
    }
    bool get_control_points_visible()
    {
        return control_point_visible_;
    }
    int get_ann_sample_n()
    {
        return ann_sample_n_;
    }
    void set_idw_mu(float mu)
    {
        idw_mu_ = mu;
    }
    void set_rbf_r(float r)
    {
        rbf_r_ = r;
    }
    void set_rbf_mu(float mu)
    {
        rbf_mu_ = mu;
    }
    void set_control_points_visible(bool visible)
    {
        control_point_visible_ = visible;
    }
    void set_ann_sample_n(int sample_n)
    {
        ann_sample_n_ = sample_n;
    }
    void add_control_points(int srcx, int srcy, int destx, int desty)
    {
        start_points_.push_back(ImVec2(srcx, srcy));
        end_points_.push_back(ImVec2(destx, desty));
    }
   private:
    // Store the original image data
    std::shared_ptr<Image> back_up_;
    // The selected point couples for image warping
    std::vector<ImVec2> start_points_, end_points_;

    ImVec2 start_, end_;
    bool flag_enable_selecting_points_ = false;
    bool draw_status_ = false;

   private:
    ImageWarpingAlgorithmType image_warping_algorithm_type_ = Warping_IDW;
    FillHoleAlgorithmType fill_hole_algorithm_type_ = FillHole_None;

    float idw_mu_ = 2;
    float rbf_r_ = 30;
    float rbf_mu_ = -1;
    bool control_point_visible_ = true;
    int ann_sample_n_ = 3;
};

}  // namespace USTC_CG