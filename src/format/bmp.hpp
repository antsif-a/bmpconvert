#pragma once

#include <istream>
#include <vector>
#include "pixel_array.hpp"

constexpr std::array<uint8_t, 2> BitmapSignature = {0x42, 0x4D};

struct BitmapFileHeader {
    uint32_t file_size;
    uint16_t reserved1 = 0, reserved2 = 0;
    uint32_t pixel_array_offset;
};

struct BitmapCoreHeader {
    uint32_t header_size;
    int32_t bitmap_width;
    int32_t bitmap_height;
    uint16_t planes, bits_per_pixel;

    enum : uint32_t {
        RGB         = 0,
        RLE8        = 1,
        RLE4        = 2,
        BITFIELDS   = 3,
        JPEG        = 4,
        PNG         = 5
    } compression;

    uint32_t image_size;
    int32_t x_pixels_per_metre;
    int32_t y_pixels_per_metre;

    uint32_t colors;
    uint32_t importrant_color_count;
};

struct BitmapV5Header : BitmapCoreHeader {
    uint32_t red_channel_bitmask;
    uint32_t green_channel_bitmask;
    uint32_t blue_channel_bitmask;
    uint32_t alpha_channel_bitmask;
    enum {
        LCS_CALIBRATED_RGB      = 0x00000000,
        LCS_sRGB                = 0x73524742,
        LCS_WINDOWS_COLOR_SPACE = 0x57696E20,
        LCS_PROFILE_LINKED      = 0x4C494E4B,
        LCS_PROFILE_EMBEDDED    = 0x4D424544
    } color_space_type;

    struct {
        vec3<float> red;
        vec3<float> green;
        vec3<float> blue;
    } color_space_endpoints;

    uint32_t gamma_for_red_channel;
    uint32_t gamma_for_green_channel;
    uint32_t gamma_for_blue_channel;

    enum {
        LCS_GM_ABS_COLORIMETRIC = 0x00000008,
        LCS_GM_BUSINESS = 0x00000001,
        LCS_GM_GRAPHICS = 0x00000002,
        LCS_GM_IMAGES = 0x00000004
    } intent;
    uint32_t profile_data;
    uint32_t profile_size;
    uint32_t reserved = 0;
};

class Bitmap {
    BitmapFileHeader file_header;

public:
    // everyone uses BITMAPV5HEADER anyway
    BitmapV5Header header;
    std::vector<vec4<uint8_t>> color_table;
    BitmapPixelArray * pixels = nullptr;

    Bitmap(BitmapV5Header file_header, BitmapV5Header header, std::vector<vec4<uint8_t>> color_table, BitmapPixelArray * pixels);

    Bitmap(std::istream & input);
    Bitmap(const char * path);

    void write(std::ostream & output);
    void write(const char * path);

    void read(std::istream & input);
    void read(const char * path);

    void rotate_90();
    void rotate(int deg);

    void cut(vec2<int> a, vec2<int> b);

    void print_info();

    void inverse_colors();
};
