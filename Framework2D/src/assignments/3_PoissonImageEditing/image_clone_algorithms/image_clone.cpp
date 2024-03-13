#include "image_clone.h"


namespace USTC_CG
{
// 这两个稍加封装的函数同时处理了三个问题：
// 1. 将颜色值转为 float， 便于计算；
// 2. 判断超界，并设置超界范围的颜色；
// 3. 提供获取邻域颜色的功能，简化代码。dir 取 1 到 4 分别对应右、左、上、下。

std::vector<float> ImageCloneAlgorithm::col_src(int x, int y, int dir = 0)
{
    const int DIRMAPX[5] = { 0, 1, -1, 0, 0 };
    const int DIRMAPY[5] = { 0, 0, 0, 1, -1 };
    x += DIRMAPX[dir];
    y += DIRMAPY[dir];
    if (x < 0)
        x = 0;
    else if (x >= data_src_->width())
        x = data_src_->width() - 1;
    if (y < 0)
        y = 0;
    else if (y >= data_src_->height())
        y = data_src_->height() - 1;
    std::vector<float> ret;
    auto pixel = data_src_->get_pixel(x, y);
    ret.resize(3, 0);
    for (int c = 0; c < 3; c++)
        ret[c] = (float)pixel[c];
    return ret;
}

std::vector<float> ImageCloneAlgorithm::col_tar(int x, int y, int dir = 0)
{
    const int DIRMAPX[5] = { 0, 1, -1, 0, 0 };
    const int DIRMAPY[5] = { 0, 0, 0, 1, -1 };
    x += DIRMAPX[dir];
    y += DIRMAPY[dir];
    if (x < 0)
        x = 0;
    else if (x >= data_tar_->width())
        x = data_tar_->width() - 1;
    if (y < 0)
        y = 0;
    else if (y >= data_tar_->height())
        y = data_tar_->height() - 1;
    std::vector<float> ret;
    auto pixel = data_tar_->get_pixel(x, y);
    ret.resize(3, 0);
    for (int c = 0; c < 3; c++)
        ret[c] = (float)pixel[c];
    return ret;
}
}  // namespace USTC_CG