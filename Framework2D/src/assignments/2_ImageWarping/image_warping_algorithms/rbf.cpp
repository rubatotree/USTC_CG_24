#include "image_warping_algorithms/rbf.h"
#include <algorithm>

int WarpingRBF::dist(int i, int j)
{
    return (samples[i].src - samples[i].dest).length_sqr();
}

double WarpingRBF::func(int base, Point src)
{
    return std::pow(r_ * r_ + (samples[base].src - src).length_sqr(), mu_ / 2);
}

Point WarpingRBF::Transform(Point src)
{
    int size = samples.size();
    if (size == 0)
        return src;
    Vector dest = Vector(0, 0);
    for (int i = 0; i < size; i++)
    {
        dest = Vector(AlphaX(i), AlphaY(i)) * (float)func(i, src) + dest;
    }
    dest = Vector(AlphaX(size), AlphaY(size)) * (float)src.x +
           Vector(AlphaX(size + 1), AlphaY(size + 1)) * (float)src.y +
           Vector(AlphaX(size + 2), AlphaY(size + 2)) + dest;

    return Point(dest.x, dest.y);
}

void WarpingRBF::Update()
{
    int size = samples.size();
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> Mat_coff(size + 3, size + 3);
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            Mat_coff(i, j) = func(j, samples[i].src);
    for (int i = 0; i < size; i++)
    {
        Mat_coff(size + 0, i) = samples[i].src.x;
        Mat_coff(size + 1, i) = samples[i].src.y;
        Mat_coff(size + 2, i) = 1;
        Mat_coff(i, size + 0) = samples[i].src.x;
        Mat_coff(i, size + 1) = samples[i].src.y;
        Mat_coff(i, size + 2) = 1;
    }
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            Mat_coff(size + i, size + j) = 0;
    Eigen::VectorXd Bx(size + 3), By(size + 3);
    for (int i = 0; i < size; i++)
    {
        Bx(i) = samples[i].dest.x;
        By(i) = samples[i].dest.y;
    }
    Bx(size) = 0;
    By(size) = 0;
    Bx(size + 1) = 0;
    By(size + 1) = 0;
    Bx(size + 2) = 0;
    By(size + 2) = 0;
    AlphaX = Mat_coff.colPivHouseholderQr().solve(Bx);
    AlphaY = Mat_coff.colPivHouseholderQr().solve(By);

    if (size == 0)
    {
        AlphaX(size) = 1;
        AlphaY(size) = 0;
        AlphaX(size + 1) = 0;
        AlphaY(size + 1) = 1;
        AlphaX(size + 2) = 0;
        AlphaY(size + 2) = 0;
    }
    if (size == 1)
    {
        AlphaX(size) = 1;
        AlphaY(size) = 0;
        AlphaX(size + 1) = 0;
        AlphaY(size + 1) = 1;
        AlphaX(size + 2) = (samples[0].dest - samples[0].src).x;
        AlphaY(size + 2) = (samples[0].dest - samples[0].src).y;
    }
    if (size == 2)
    {
        AlphaX(0) = AlphaY(0) = 0;
        AlphaX(1) = AlphaY(1) = 0;
        AlphaX(2) = (samples[1].dest - samples[0].dest).x / (samples[1].src - samples[0].src).x;
        AlphaY(2) = 0;
        AlphaX(3) = 0;
        AlphaY(3) = (samples[1].dest - samples[0].dest).y / (samples[1].src - samples[0].src).y;
        AlphaX(4) = samples[0].dest.x - AlphaX(2) * samples[0].src.x;
        AlphaY(4) = samples[0].dest.y - AlphaY(3) * samples[0].src.y;
    }
}