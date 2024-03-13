#include "seamless.h"
#include <Eigen/Dense>

void USTC_CG::ImageCloneSeamless::clone()
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
            // Dirichlet 边界条件
            auto col = col_tar(ptar.x, ptar.y, 0);
            for (int c = 0; c < 3; c++)
                F[c](i) = col[c];
        }
        else if (type == PointSet::kIn)
        {
            // Laplacian
            auto col = col_src(psrc.x, psrc.y, 0);

            for (int c = 0; c < 3; c++)
                col[c] *= -4;
            for (int d = 1; d <= 4; d++)
            {
                auto col_d = col_src(psrc.x, psrc.y, d);
                for (int c = 0; c < 3; c++)
                    col[c] += col_d[c];
            }

            for (int c = 0; c < 3; c++)
                F[c](i) = col[c];
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