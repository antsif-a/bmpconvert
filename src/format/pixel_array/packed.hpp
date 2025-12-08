#pragma once

#include <cstdint>
#include <functional>
#include "format/pixel_array.hpp"
#include "math/matrix.hpp"

matrix<uint8_t> build_packed_pixel_matrix(
    uint16_t bits_per_pixel,
    unsigned int width,
    int height,
    std::vector<color> & color_table,
    std::function<uint8_t (unsigned int i, unsigned int j)> get_pixel
);

class PackedBitmapPixelArray : public BitmapPixelArray {
public:
    uint16_t bits_per_pixel;
    unsigned int w; int h;
    unsigned int pixels_per_byte;
    unsigned int row_size;
    unsigned int pixel_array_size_in_bytes;
    bool height_signed;
    matrix<uint8_t> pixels;
    std::vector<color> & color_table;

    PackedBitmapPixelArray(
        uint16_t bits_per_pixel,
        unsigned int width,
        int height,
        std::vector<color> & color_table
    );

    unsigned int width() override;
    int height() override;

    color get_pixel(unsigned int i, unsigned int j) override;
    uint8_t get_pixel_color_idx(unsigned int i, unsigned int j);

    void rotate_90() override;
    void cut(vec2<unsigned int> a, vec2<unsigned int> b) override;

    uint8_t * data() override;
    size_t byte_size() override;
    int row_byte_size() override;
};