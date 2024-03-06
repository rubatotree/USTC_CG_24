#include "image_warping_algorithms/idw.h"
#include <Eigen/Dense>

Point WarpingIDW::Transform(Point src)
{
    if (samples.size() == 0)
        return src;

    float sigma_tot = 0;
    Point cur_src = src;
    Vector final_dest = Vector(0, 0);
    for (int i = 0; i < samples.size(); i++)
    {
        Point src = samples[i].src;
        Point dest = samples[i].dest;

        float sigma = 1 / pow((cur_src - dest).length(), mu_);
        sigma_tot += sigma;
        final_dest = (dest + transform_matrix_[i] * (cur_src - src)) * sigma - Point(0, 0) + final_dest;
    }

    final_dest = final_dest / sigma_tot;
    return Point(final_dest.x, final_dest.y);
}

void WarpingIDW::Update()
{
    int size = samples.size();
    transform_matrix_.resize(size, Matrix22(1, 0, 0, 1));
    if (size < 3)
        return;
    for (int i = 0; i < size; i++)
    {
        Eigen::Matrix4f mat_coff;
        Eigen::Vector4f vec_consts;
        Eigen::Vector4f vec_ds;
        mat_coff.setZero();
        vec_consts.setZero();
        for (int j = 0; j < size; j++)
        {
            if (j == i) continue;
            float sigma = 1 / pow((samples[i].src - samples[j].src).length(), mu_);
            // 构造求导后的方程组
            mat_coff(0, 0) += (samples[j].src.x - samples[i].src.x) * sigma * (samples[j].src.x - samples[i].src.x);
            mat_coff(1, 0) += (samples[j].src.y - samples[i].src.y) * sigma * (samples[j].src.x - samples[i].src.x);
            vec_consts(0) += -(samples[i].dest.x - samples[j].dest.x) * sigma * (samples[j].src.x - samples[i].src.x);
            
            mat_coff(0, 1) += (samples[j].src.x - samples[i].src.x) * sigma * (samples[j].src.y - samples[i].src.y);
            mat_coff(1, 1) += (samples[j].src.y - samples[i].src.y) * sigma * (samples[j].src.y - samples[i].src.y);
            vec_consts(1) += -(samples[i].dest.x - samples[j].dest.x) * sigma * (samples[j].src.y - samples[i].src.y);

            mat_coff(2, 2) += (samples[j].src.x - samples[i].src.x) * sigma * (samples[j].src.x - samples[i].src.x);
            mat_coff(3, 2) += (samples[j].src.y - samples[i].src.y) * sigma * (samples[j].src.x - samples[i].src.x);
            vec_consts(2) += -(samples[i].dest.y - samples[j].dest.y) * sigma * (samples[j].src.x - samples[i].src.x);

            mat_coff(2, 3) += (samples[j].src.x - samples[i].src.x) * sigma * (samples[j].src.y - samples[i].src.y);
            mat_coff(3, 3) += (samples[j].src.y - samples[i].src.y) * sigma * (samples[j].src.y - samples[i].src.y);
            vec_consts(3) += -(samples[i].dest.y - samples[j].dest.y) * sigma * (samples[j].src.y - samples[i].src.y);
        }
        vec_ds = mat_coff.fullPivLu().solve(vec_consts);
        transform_matrix_[i] = Matrix22(vec_ds(0), vec_ds(1), vec_ds(2), vec_ds(3));
    }
}