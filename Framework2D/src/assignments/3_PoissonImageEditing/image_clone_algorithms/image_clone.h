#pragma once

#include "point_set.h"
#include "view/image.h"

namespace USTC_CG
{
class ImageCloneAlgorithm
{
   public:
    ImageCloneAlgorithm
        (PointSet* point_set,
        std::shared_ptr<Image> data_src,
        std::shared_ptr<Image> data_tar,
        PointI offset)
        : 
          point_set_(point_set),
          data_src_(data_src),
          data_tar_(data_tar),
          offset_(offset)
    {

    }
    virtual void clone() = 0;

   protected:
    std::shared_ptr<Image> data_src_;
    std::shared_ptr<Image> data_tar_;
    PointSet* point_set_ = nullptr;
    PointI offset_;

    std::vector<float> col_src(int x, int y, int dir);
    std::vector<float> col_tar(int x, int y, int dir);
};
}