#include "format/pixel_array/expanded.hpp"
#include <cassert>

inline unsigned int ExpandedBitmapPixelArray::tz_count(uint32_t v) {
    if (v == 0) return 32;
    return __builtin_ctz(v);
}
inline unsigned int ExpandedBitmapPixelArray::bit_count(uint32_t v) {
    return __builtin_popcount(v);
}

ExpandedBitmapPixelArray::ExpandedBitmapPixelArray(
    uint16_t bits_per_pixel,
    unsigned int width_,
    int height_,
    std::vector<color>& color_table_,
    uint32_t rmask,
    uint32_t gmask,
    uint32_t bmask
) :
    bits_per_pixel(bits_per_pixel),
    w(width_), h(height_),
    bytes_per_pixel(bits_per_pixel / 8),
    row_size(static_cast<unsigned int>(get_row_size(bits_per_pixel, width_))),
    pixel_array_size_in_bytes(static_cast<unsigned int>(get_pixel_array_size(row_size, height_))),
    height_signed(height_ > 0),
    pixels(static_cast<unsigned int>(std::abs(height_)), row_size),
    color_table(color_table_),
    red_mask(rmask), green_mask(gmask), blue_mask(bmask)
{
    assert(bits_per_pixel == 8 || bits_per_pixel == 16 || bits_per_pixel == 24);
    assert(bytes_per_pixel * 8 == bits_per_pixel);
}

unsigned int ExpandedBitmapPixelArray::width() { return w; }
int ExpandedBitmapPixelArray::height() { return h; }

uint8_t * ExpandedBitmapPixelArray::data() {
    return pixels.data();
}

size_t ExpandedBitmapPixelArray::byte_size() {
    return pixels.rows() * pixels.columns();
}

int ExpandedBitmapPixelArray::row_byte_size() {
    return row_size;
}

// converts a 16-bit pixel value to color, using masks if provided or default 5-6-5 otherwise
color ExpandedBitmapPixelArray::color_from_16bit(uint16_t v) const {
    uint32_t rmask_local = red_mask;
    uint32_t gmask_local = green_mask;
    uint32_t bmask_local = blue_mask;

    if (rmask_local == 0 && gmask_local == 0 && bmask_local == 0) {
        // assume RGB565 (most common)
        rmask_local = 0xF800u;
        gmask_local = 0x07E0u;
        bmask_local = 0x001Fu;
    }

    // extract and scale to 0-255
    auto extract_and_scale = [](uint32_t value, uint32_t mask)->uint8_t {
        if (mask == 0) return 0;
        unsigned int shift = __builtin_ctz(mask);
        unsigned int bits = __builtin_popcount(mask);
        uint32_t raw = (value & mask) >> shift;
        uint32_t maxv = (1u << bits) - 1u;
        // scale raw (0..maxv) to 0..255
        uint32_t scaled = (raw * 255u + (maxv/2u)) / maxv;
        return static_cast<uint8_t>(scaled);
    };

    uint32_t val = static_cast<uint32_t>(v);
    uint8_t r = extract_and_scale(val, rmask_local);
    uint8_t g = extract_and_scale(val, gmask_local);
    uint8_t b = extract_and_scale(val, bmask_local);

    return color{ r, g, b, 255u };
}

color ExpandedBitmapPixelArray::get_pixel(unsigned int i, unsigned int j) {
    // map logical row i (top-down) to stored row
    unsigned int rows = pixels.rows();
    unsigned int row_index = (height_signed ? (rows - 1 - i) : i);

    // compute byte offset into the scanline
    unsigned int byte_index = j * bytes_per_pixel;
    // bounds safety (shouldn't happen if caller validated)
    if (row_index >= rows || byte_index + bytes_per_pixel > pixels.columns()) {
        return color{0,0,0,0};
    }

    // read bytes (BMP stores little-endian per pixel component and BGR order for 24bpp)
    if (bits_per_pixel == 8) {
        uint8_t idx = pixels(row_index, byte_index);
        // color_table should exist (palette)
        if (idx < color_table.size()) return color_table[idx];
        return color{0,0,0,255};
    } else if (bits_per_pixel == 16) {
        uint16_t lo = pixels(row_index, byte_index);
        uint16_t hi = pixels(row_index, byte_index + 1);
        uint16_t val = static_cast<uint16_t>((hi << 8) | lo); // little-endian
        return color_from_16bit(val);
    } else { // 24 bpp
        uint8_t b = pixels(row_index, byte_index + 0);
        uint8_t g = pixels(row_index, byte_index + 1);
        uint8_t r = pixels(row_index, byte_index + 2);
        return color{ r, g, b, 255u };
    }
}

void ExpandedBitmapPixelArray::rotate_90() {
    int new_h = height_signed ? -static_cast<int>(w) : static_cast<int>(w);
    unsigned int new_w = static_cast<unsigned int>(std::abs(h));

    unsigned int new_row_size = static_cast<unsigned int>(get_row_size(bits_per_pixel, new_w));
    unsigned int new_pixel_array_size = static_cast<unsigned int>(get_pixel_array_size(new_row_size, new_h));

    // build new pixels matrix
    matrix<uint8_t> new_pixels(static_cast<unsigned int>(std::abs(new_h)), new_row_size);

    // For each destination pixel at (i,j) write the corresponding source pixel
    for (unsigned int i = 0; i < static_cast<unsigned int>(std::abs(new_h)); ++i) {
        for (unsigned int j = 0; j < new_w; ++j) {
            // source coordinate mapping for 90 deg clockwise:
            // src_y = new_w - 1 - j
            // src_x = i
            color c = get_pixel(new_w - 1 - j, i);

            // write color c to new_pixels row i at column j (bytes_per_pixel)
            unsigned int dest_byte = j * bytes_per_pixel;
            // ensure we don't exceed the row (padding sits after pixel bytes)
            if (dest_byte + bytes_per_pixel > new_pixels.columns()) continue;

            if (bits_per_pixel == 8) {
                // palette index not available here — try to match exact color in table (fallback)
                // Prefer storing the index if possible: search color_table
                uint8_t idx = 0;
                bool found = false;
                for (uint32_t k = 0; k < color_table.size(); ++k) {
                    if (color_table[k] == c) { idx = static_cast<uint8_t>(k); found = true; break; }
                }
                new_pixels(i, dest_byte) = idx;
            } else if (bits_per_pixel == 16) {
                // pack using masks if available, otherwise RGB565
                // naive packing: scale 8-bit components down to bit ranges then place into 16-bit value
                uint32_t rmask_local = red_mask, gmask_local = green_mask, bmask_local = blue_mask;
                uint16_t packed = 0;
                if (rmask_local == 0 && gmask_local == 0 && bmask_local == 0) {
                    // RGB565 default
                    uint16_t r5 = static_cast<uint16_t>((c[0] * 31 + 127) / 255) & 0x1F;
                    uint16_t g6 = static_cast<uint16_t>((c[1] * 63 + 127) / 255) & 0x3F;
                    uint16_t b5 = static_cast<uint16_t>((c[2] * 31 + 127) / 255) & 0x1F;
                    packed = static_cast<uint16_t>((r5 << 11) | (g6 << 5) | b5);
                } else {
                    // use masks: find shift and width for each
                    auto pack_comp = [&](uint8_t comp8, uint32_t mask)->uint32_t {
                        if (mask == 0) return 0;
                        unsigned int shift = tz_count(mask);
                        unsigned int bits = bit_count(mask);
                        uint32_t maxv = (1u<<bits)-1u;
                        uint32_t small = (static_cast<uint32_t>(comp8) * maxv + 127u) / 255u;
                        return (small << shift) & mask;
                    };
                    uint32_t v = pack_comp(c[0], rmask_local) | pack_comp(c[1], gmask_local) | pack_comp(c[2], bmask_local);
                    packed = static_cast<uint16_t>(v & 0xFFFFu);
                }
                // store little-endian
                new_pixels(i, dest_byte + 0) = static_cast<uint8_t>(packed & 0xFFu);
                new_pixels(i, dest_byte + 1) = static_cast<uint8_t>((packed >> 8) & 0xFFu);
            } else { // 24 bpp
                new_pixels(i, dest_byte + 0) = c[2]; // B
                new_pixels(i, dest_byte + 1) = c[1]; // G
                new_pixels(i, dest_byte + 2) = c[0]; // R
            }
        }
    }

    pixels = std::move(new_pixels);
    w = new_w;
    h = new_h;
    row_size = new_row_size;
    pixel_array_size_in_bytes = new_pixel_array_size;
    height_signed = (h > 0);
}

void ExpandedBitmapPixelArray::cut(vec2<unsigned int> a, vec2<unsigned int> b) {
    assert(a[0] <= b[0]);
    assert(a[1] <= b[1]);

    unsigned int new_w = b[0] - a[0] + 1u;
    int raw_new_h = static_cast<int>(b[1] - a[1] + 1u);
    int new_h = height_signed ? -raw_new_h : raw_new_h;

    unsigned int new_row_size = static_cast<unsigned int>(get_row_size(bits_per_pixel, new_w));
    unsigned int new_pixel_array_size = static_cast<unsigned int>(get_pixel_array_size(new_row_size, new_h));

    matrix<uint8_t> new_pixels(static_cast<unsigned int>(std::abs(new_h)), new_row_size);

    // For each pixel in new image, copy from source
    for (unsigned int i = 0; i < static_cast<unsigned int>(std::abs(new_h)); ++i) {
        unsigned int src_y = a[1] + i;
        for (unsigned int j = 0; j < new_w; ++j) {
            unsigned int src_x = a[0] + j;
            // read source pixel bytes and copy them into dest row
            unsigned int src_row = height_signed ? (pixels.rows() - 1 - src_y) : src_y;
            unsigned int src_byte = src_x * bytes_per_pixel;
            unsigned int dst_byte = j * bytes_per_pixel;

            // copy bytes_per_pixel bytes (beware of row padding)
            for (unsigned int b = 0; b < bytes_per_pixel; ++b) {
                uint8_t v = pixels(src_row, src_byte + b);
                new_pixels(i, dst_byte + b) = v;
            }
        }
    }

    pixels = std::move(new_pixels);
    w = new_w;
    h = new_h;
    row_size = new_row_size;
    pixel_array_size_in_bytes = new_pixel_array_size;
}
