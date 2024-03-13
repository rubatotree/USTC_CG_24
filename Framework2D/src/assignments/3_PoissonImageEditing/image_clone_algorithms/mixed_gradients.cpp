#include <Eigen/Dense>

#include "mixed_gradients.h"

void USTC_CG::ImageCloneMixedGradients::clone()
{
    int size = point_set_->size();
    Eigen::VectorXf F[3] = { Eigen::VectorXf(size),
                             Eigen::VectorXf(size),
                             Eigen::VectorXf(size) },
                    G[3] = { Eigen::VectorXf(size),
                             Eigen::VectorXf(size),
                             Eigen::VectorXf(size) };
    for (int i = 0; i < size; i++)
    {
        PointI psrc = point_set_->get_point_at(i);
        PointI ptar = psrc + offset_;
        PointSet::PointType type = point_set_->check(psrc);

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
    }
    for (int c = 0; c < 3; c++)
        G[c] = point_set_->solve(F[c]);

    for (int i = 0; i < size; i++)
    {
        PointI ptar = point_set_->get_point_at(i) + offset_;
        if (ptar.x < 0 || ptar.x >= data_tar_->width() || ptar.y < 0 ||
            ptar.y >= data_tar_->height())
            continue;

        // 防止颜色值超界
        unsigned char r = std::clamp(G[0](i), (float)0, (float)255);
        unsigned char g = std::clamp(G[1](i), (float)0, (float)255);
        unsigned char b = std::clamp(G[2](i), (float)0, (float)255);
        data_tar_->set_pixel(ptar.x, ptar.y, { r, g, b });
    }
}