#include "path.h"

#include <cfloat>
#include <iostream>
#include <random>

#include "surfaceInteraction.h"
#include "utils/sampling.hpp"
USTC_CG_NAMESPACE_OPEN_SCOPE
using namespace pxr;

VtValue PathIntegrator::Li(const GfRay& ray, std::default_random_engine& random)
{
    std::uniform_real_distribution<float> uniform_dist(
        0.0f, 1.0f - std::numeric_limits<float>::epsilon());
    std::function<float()> uniform_float = std::bind(uniform_dist, random);

    auto color = EstimateOutGoingRadiance(ray, uniform_float, 0);

    return VtValue(GfVec3f(color[0], color[1], color[2]));
}

GfVec3f PathIntegrator::EstimateOutGoingRadiance(
    const GfRay& ray,
    const std::function<float()>& uniform_float,
    int recursion_depth)
{
    if (recursion_depth >= 50) {
        return {};
    }

    SurfaceInteraction si;
    if (!Intersect(ray, si)) {
        if (recursion_depth == 0) {
            return IntersectDomeLight(ray);
        }

        return GfVec3f{ 0, 0, 0 };
    }

    // This can be customized : Do we want to see the lights? (Other than dome lights?)
    const bool see_lights = false;
    if (see_lights && recursion_depth == 0) {
        GfVec3f pos = {NAN, NAN, NAN};
        auto light = IntersectLights(ray, pos);
        if (!isnan(pos[0]))
            return light;
    }

    // Flip the normal if opposite
    if (GfDot(si.shadingNormal, ray.GetDirection()) > 0) {
        si.flipNormal();
        si.PrepareTransforms();
    }

    GfVec3f color{ 0 };
    GfVec3f directLight = EstimateDirectLight(si, uniform_float);

    // Russian roulette
    const float russian_roulette_probability = 0.8f;
    if (uniform_float() > russian_roulette_probability)
        return directLight;

    // Global Lighting
    GfVec3f globalLight;
    GfVec3f wi;
    Color fr;
    float pdf;

    si.Sample(wi, pdf, uniform_float);
    auto li = EstimateOutGoingRadiance( GfRay(si.position + 0.0001f * si.geometricNormal, wi), uniform_float, recursion_depth + 1);
    fr = si.Eval(wi);
    float cosVal = abs(GfDot(si.shadingNormal, wi));
    globalLight = GfCompMult(li, fr) * cosVal / pdf;

    color = directLight + globalLight;
    return color;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE
