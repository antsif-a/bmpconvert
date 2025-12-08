#include <cmath>

int get_row_size(unsigned int bits_per_pixel, unsigned int image_width) {
    return ceil(((float) bits_per_pixel * (float) image_width) / 32) * 4;
}

int get_pixel_array_size(unsigned int row_size, int image_height) {
    return row_size * abs(image_height);
}