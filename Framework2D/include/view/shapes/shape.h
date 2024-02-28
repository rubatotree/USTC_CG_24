#pragma once

#include "imgui.h"

namespace USTC_CG
{
class Shape
{
   public:
    // Draw Settings
    struct Config
    {
        // Line color in RGBA format
        unsigned char line_color[4] = { 0, 0, 0, 255 };
        float line_thickness = 2.0f;
        bool fill = false;
        Config() { }
        Config(bool fill, float thickness, float color[3])
            : fill(fill),
            line_thickness(thickness)
        {
            line_color[0] = (unsigned char)(color[0] * 255);
            line_color[1] = (unsigned char)(color[1] * 255);
            line_color[2] = (unsigned char)(color[2] * 255);
        }
    };

   public:
    virtual ~Shape() = default;
    /**
     * Draws the shape on the screen.
     * This is a pure virtual function that must be implemented by all derived
     * classes.
     *
     * @param config The configuration settings for drawing, including line
     * color, thickness, and bias.
     *               - line_color defines the color of the shape's outline.
     *               - line_thickness determines how thick the outline will be.
     *               - bias is used to adjust the shape's position on the
     * screen.
     */
    // 传参不再传递 Config，而是只传递 bias
    virtual void draw(float bias[2]) const = 0;
    /**
     * Updates the state of the shape.
     * This function allows for dynamic modification of the shape, in response
     * to user interactions like dragging.
     *
     * @param x, y Dragging point. e.g. end point of a line.
     */
    virtual void update(float x, float y) = 0;
    void setConfig(const Config& config)
    {
        config_ = config;
    }
protected:
    // 为每个shape存储单独的config
    Config config_ = Config();
};
}  // namespace USTC_CG