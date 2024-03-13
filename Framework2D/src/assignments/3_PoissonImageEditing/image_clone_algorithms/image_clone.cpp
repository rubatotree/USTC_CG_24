#include "image_clone.h"


namespace USTC_CG
{
// �������Լӷ�װ�ĺ���ͬʱ�������������⣺
// 1. ����ɫֵתΪ float�� ���ڼ��㣻
// 2. �жϳ��磬�����ó��緶Χ����ɫ��
// 3. �ṩ��ȡ������ɫ�Ĺ��ܣ��򻯴��롣dir ȡ 1 �� 4 �ֱ��Ӧ�ҡ����ϡ��¡�

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