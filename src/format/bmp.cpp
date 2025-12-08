#include <cassert>
#include <print>
#include <fstream>
#include "bmp.hpp"
#include "exceptions.hpp"
#include "format/pixel_array/expanded.hpp"
#include "format/pixel_array/packed.hpp"

namespace io {
    template<typename T>
    void read(std::istream & is, T * value)  {
        is.read(reinterpret_cast<char *>(value), sizeof(T));
    }

    template<typename T>
    void read(std::istream & is, T * value, std::streamsize size)  {
        is.read(reinterpret_cast<char *>(value), size);
    }

    template<typename T>
    void write(std::ostream & os, T * value) {
        os.write(reinterpret_cast<char *>(value), sizeof(T));
    }

    template<typename T>
    void write(std::ostream & os, T * value, std::streamsize size) {
        os.write(reinterpret_cast<char *>(value), size);
    }

    template<typename T>
    void write(std::ostream & os, const T * value, std::streamsize size) {
        os.write(reinterpret_cast<const char *>(value), size);
    }
}

Bitmap::Bitmap(std::istream & input) {
    read(input);
}

Bitmap::Bitmap(const char * path) {
    read(path);
}

void Bitmap::read(std::istream & input) {
    std::array<std::uint8_t, 2> signature;
    io::read(input, signature.data(), signature.size() * sizeof(signature[0]));

    if (!std::equal(BitmapSignature.begin(), BitmapSignature.end(), signature.begin())) {
        throw not_a_bmp_file();
    }

    io::read(input, &file_header.file_size);
    input.seekg(4, std::ios_base::cur); // skip reserved fields
    io::read(input, &file_header.pixel_array_offset);

    uint32_t header_size;
    io::read(input, &header_size);

    assert(header_size == sizeof(BitmapV5Header));
    header.header_size = header_size;
    io::read(input, &header.bitmap_width, header_size - sizeof(header.header_size));

    color_table = std::vector<vec4<uint8_t>>(header.colors);
    io::read(input, color_table.data(), sizeof(vec4<uint8_t>) * header.colors);

    if (header.bits_per_pixel < 8) {
        pixels = new PackedBitmapPixelArray(header.bits_per_pixel, header.bitmap_width, header.bitmap_height, color_table);
    } else {
        uint32_t rmask = header.red_channel_bitmask;
        uint32_t gmask = header.green_channel_bitmask;
        uint32_t bmask = header.blue_channel_bitmask;
        
        pixels = new ExpandedBitmapPixelArray(header.bits_per_pixel, header.bitmap_width, header.bitmap_height, color_table, rmask, gmask, bmask);
    }

    io::read(input, pixels->data(), pixels->byte_size());
}

void Bitmap::read(const char * path) {
    std::ifstream is(path);
    if (!is.is_open()) {
        throw invalid_file_path(path);
    }
    read(is);
}

void Bitmap::write(std::ostream & os) {
    io::write(os, BitmapSignature.data(), BitmapSignature.size());
    io::write(os, &file_header);
    io::write(os, &header);
    io::write(os, color_table.data(), color_table.size() * sizeof(color_table[0]));
    io::write(os, pixels->data(), pixels->byte_size());
}

void Bitmap::write(const char * path) {
    std::ofstream os(path);
    if (!os.is_open()) {
        throw invalid_file_path(path);
    }
    write(os);
}

void Bitmap::rotate_90() {
    file_header.file_size -= pixels->byte_size();
    pixels->rotate_90();
    header.bitmap_width  = static_cast<int32_t>(pixels->width());
    header.bitmap_height = static_cast<int32_t>(pixels->height()); // contains sign for top-down vs bottom-up
    file_header.file_size += pixels->byte_size();
}

void Bitmap::rotate(int deg) {
    if ((abs(deg) % 90) != 0) {
        throw invalid_degrees(deg);
    }
    if (deg < 0) {
        deg = 360 - 90;
    }
    deg %= 360;
    for (int i = 0; i < deg / 90; i++) {
        rotate_90();
    }
}

void Bitmap::cut(vec2<int> a, vec2<int> b) {
    if (a[0] > b[0] || a[1] > b[1]
            || a[0] < 0 || a[1] < 0 || b[0] < 0 || b[1] < 0
            || a[0] > header.bitmap_width
            || b[0] > header.bitmap_height) {
        throw invalid_coordinates(a, b);
    }

    vec2<unsigned int> ua {static_cast<unsigned int>(a[0]), static_cast<unsigned int>(a[1])};
    vec2<unsigned int> ub {static_cast<unsigned int>(b[0]), static_cast<unsigned int>(b[1])};

    file_header.file_size -= pixels->byte_size();
    pixels->cut(ua, ub);
    header.bitmap_width  = static_cast<int32_t>(pixels->width());
    header.bitmap_height = static_cast<int32_t>(pixels->height()); // contains sign for top-down vs bottom-up
    file_header.file_size += pixels->byte_size();
}


void Bitmap::print_info() {
    std::println("file size: {}", file_header.file_size);
    std::println("bitmap size: {}x{} pixels", header.bitmap_width, abs(header.bitmap_height));
    std::println("bits per pixel: {}", header.bits_per_pixel);
}

void Bitmap::inverse_colors() {
    if (header.bits_per_pixel <= 8) {
        for (auto & color : color_table) {
            for (int i = 0; i < 3; i++) {
                color[i] = 255 - color[i];
            }
        }
    } 

    if (header.bits_per_pixel == 16) {
        auto *data = reinterpret_cast<uint16_t *>(pixels->data());
        size_t num_pixels = pixels->width() * abs(pixels->height());
        for (size_t i = 0; i < num_pixels; ++i) {
            uint16_t pixel = data[i];
            uint8_t r = 31 - ((pixel >> 11) & 0x1F);
            uint8_t g = 63 - ((pixel >> 5) & 0x3F);
            uint8_t b = 31 - (pixel & 0x1F);
            data[i] = (r << 11) | (g << 5) | b;
        }
    }

    if (header.bits_per_pixel == 24) {
        uint8_t *data = pixels->data();
        size_t num_pixels = pixels->width() * abs(pixels->height());
        for (size_t i = 0; i < num_pixels; ++i) {
            data[i*3 + 0] = 255 - data[i*3 + 0]; // blue
            data[i*3 + 1] = 255 - data[i*3 + 1]; // green
            data[i*3 + 2] = 255 - data[i*3 + 2]; // red
        }
    }

}