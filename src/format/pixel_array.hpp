#pragma once

#include <cstddef>
#include <cstdint>
#include "math/vec.hpp"

using color = vec4<uint8_t>;

int get_row_size(uint32_t bits_per_pixel, uint32_t image_width);
int get_pixel_array_size(uint32_t row_size, int32_t image_height);

class BitmapPixelArray {
public:
    virtual color get_pixel(unsigned int i, unsigned int j) = 0;
    virtual unsigned int width() = 0;
    virtual int height() = 0;

    virtual void rotate_90() = 0;

    virtual void cut(vec2<unsigned int> a, vec2<unsigned int> b) = 0;

    virtual uint8_t * data() = 0;
    virtual size_t byte_size() = 0;

    virtual int row_byte_size() = 0;
};
