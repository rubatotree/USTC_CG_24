#pragma once

#include "USTC_CG.h"
#include "color.h"
#include "material.h"

using namespace USTC_CG;
Color BRDF(
    Hd_USTC_CG_Material::MaterialRecord mat,
    GfVec3f L,
    GfVec3f V,
    GfVec3f N,
    GfVec3f X,
    GfVec3f Y);
