#pragma once

#include "image_clone.h"

namespace USTC_CG
{
class ImageClonePaste : public ImageCloneAlgorithm
{
   public:
    ImageClonePaste(
        PointSet* point_set,
        std::shared_ptr<Image> data_src,
        std::shared_ptr<Image> data_tar,
        PointI offset)
        : ImageCloneAlgorithm(point_set, data_src, data_tar, offset)
    {
    }
    void clone() override;
};
}