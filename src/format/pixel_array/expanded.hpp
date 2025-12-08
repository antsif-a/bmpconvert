// format/pixel_array/expanded.hpp
#pragma once

#include <cstdint>
#include <vector>
#include "format/pixel_array.hpp"
#include "math/matrix.hpp"

class ExpandedBitmapPixelArray : public BitmapPixelArray {
public:
    uint16_t bits_per_pixel;
    unsigned int w; int h;
    unsigned int bytes_per_pixel;
    unsigned int row_size;
    unsigned int pixel_array_size_in_bytes;
    bool height_signed;
    matrix<uint8_t> pixels;               // rows = abs(h), cols = row_size (bytes)
    std::vector<color> & color_table;

    // optional masks (for 16bpp); pass 0 to use defaults for 16bpp
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;

    ExpandedBitmapPixelArray(
        uint16_t bits_per_pixel,
        unsigned int width,
        int height,
        std::vector<color>& color_table,
        uint32_t red_mask = 0,
        uint32_t green_mask = 0,
        uint32_t blue_mask = 0
    );

    unsigned int width() override;
    int height() override;

    color get_pixel(unsigned int i, unsigned int j) override;
    
    void rotate_90() override;
    void cut(vec2<unsigned int> a, vec2<unsigned int> b) override;

    uint8_t * data() override;
    size_t byte_size() override;
    int row_byte_size() override;

private:
    // helpers
    color color_from_16bit(uint16_t v) const;
    static inline unsigned int tz_count(uint32_t v); // count trailing zeros
    static inline unsigned int bit_count(uint32_t v); // count bits in mask
};
