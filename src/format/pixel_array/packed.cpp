
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include "math/matrix.hpp"
#include "format/pixel_array/packed.hpp"

matrix<uint8_t> build_packed_pixel_matrix(
    uint16_t bits_per_pixel,
    unsigned int width,
    int height,
    std::vector<color> & color_table,
    std::function<uint8_t (unsigned int i, unsigned int j)> get_pixel
) {
    assert(bits_per_pixel < 8);
    int row_size = get_row_size(bits_per_pixel, width);
    int pixels_per_byte = 8 / bits_per_pixel;
    
    matrix<uint8_t> pixels(abs(height), row_size);
    for (int i = 0; i < abs(height); i++) {
        std::vector<uint8_t> row(row_size, 0);
        for (unsigned int j = 0; j < width; j++) {
            uint8_t ij = get_pixel(i, j);
            unsigned int byte_index = j / pixels_per_byte;
            unsigned int pixel_number = j % pixels_per_byte;
            unsigned int shift = (pixels_per_byte - 1 - pixel_number) * bits_per_pixel;
            row[byte_index] |= (ij << shift);
        }
        pixels.set_row(i, row);
    }
    return pixels;
}

PackedBitmapPixelArray::PackedBitmapPixelArray(uint16_t bits_per_pixel, unsigned int width, int height, std::vector<color> & color_table)
    : bits_per_pixel(bits_per_pixel)
    , w(width), h(height)
    , pixels_per_byte(8 / bits_per_pixel)
    , height_signed(height > 0)
    , row_size(get_row_size(bits_per_pixel, width))
    , pixel_array_size_in_bytes(get_pixel_array_size(row_size, height))
    , pixels(pixel_array_size_in_bytes / row_size, row_size)
    , color_table(color_table) {
}

uint8_t PackedBitmapPixelArray::get_pixel_color_idx(unsigned int i, unsigned int j) {
    unsigned int byte_index = j / pixels_per_byte;
    
    unsigned int rows = pixels.rows();
    unsigned int row_index = (height_signed > 0) ? (rows - 1 - i) : i;
    auto raw = pixels(row_index, byte_index);

    unsigned int pixel_number = j % pixels_per_byte;
    unsigned int shift = (pixels_per_byte - 1 - pixel_number) * bits_per_pixel;
    uint8_t mask = (1u << bits_per_pixel) - 1u;

    return (raw >> shift) & mask;
}


color PackedBitmapPixelArray::get_pixel(unsigned int i, unsigned int j) {
    return color_table[get_pixel_color_idx(i, j)];
}

unsigned int PackedBitmapPixelArray::width() {
    return w;
}

int PackedBitmapPixelArray::height() {
    return h;
}

void PackedBitmapPixelArray::rotate_90() {
    int new_h = height_signed ? -((int)w) : (int)w;
    unsigned int new_w = abs(h);

    unsigned int new_row_size = get_row_size(bits_per_pixel, new_w);
    unsigned int new_pixel_array_size = get_pixel_array_size(new_row_size, new_h);

    matrix<uint8_t> new_pixels = build_packed_pixel_matrix(bits_per_pixel, new_w, new_h, color_table, [&](unsigned int i, unsigned int j) {
        return get_pixel_color_idx(new_w - 1 - j, i);
    });

    pixels = new_pixels;
    w = new_w;
    h = new_h;
    row_size = new_row_size;
    pixel_array_size_in_bytes = new_pixel_array_size;
    height_signed = (h > 0);
}

void PackedBitmapPixelArray::cut(vec2<unsigned int> a, vec2<unsigned int> b) {
    unsigned int new_w = b[0] - a[0] + 1u;
    int raw_new_h = static_cast<int>(b[1] - a[1] + 1u);
    int new_h = height_signed ? -raw_new_h : raw_new_h;

    matrix<uint8_t> new_pixels = build_packed_pixel_matrix(bits_per_pixel, new_w, new_h, color_table, [&](unsigned int i, unsigned int j) {
        return get_pixel_color_idx(a[1] + i, a[0] + j);
    });

    unsigned int new_row_size = get_row_size(bits_per_pixel, new_w);
    unsigned int new_pixel_array_size = get_pixel_array_size(new_row_size, new_h);

    pixels = std::move(new_pixels);
    w = new_w;
    h = new_h;
    row_size = new_row_size;
    pixel_array_size_in_bytes = new_pixel_array_size;
}


uint8_t * PackedBitmapPixelArray::data() {
    return pixels.data();
}

size_t PackedBitmapPixelArray::byte_size() {
    return pixel_array_size_in_bytes;
}

int PackedBitmapPixelArray::row_byte_size() {
    return row_size;
}