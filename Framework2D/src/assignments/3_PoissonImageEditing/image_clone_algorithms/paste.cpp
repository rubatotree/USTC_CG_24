#include "paste.h"

void USTC_CG::ImageClonePaste::clone()
{
    int size = point_set_->size();
    for (int i = 0; i < size; i++)
    {
        PointI psrc = point_set_->get_point_at(i);
        PointI ptar = psrc + offset_;
        PointSet::PointType type = point_set_->check(psrc);

        if (ptar.x < 0 || ptar.x >= data_tar_->width() || ptar.y < 0 ||
            ptar.y >= data_tar_->height())
            continue;
        data_tar_->set_pixel(
            ptar.x,
            ptar.y,
            data_src_->get_pixel(psrc.x, psrc.y));
    }
}