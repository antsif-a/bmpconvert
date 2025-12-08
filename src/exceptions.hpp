#pragma once

#include <stdexcept>
#include <format>

#include "math/vec.hpp"

using namespace std;

class not_a_bmp_file : public logic_error {
    public: not_a_bmp_file() : logic_error("not a bmp file") {}
};

class invalid_file_path : public invalid_argument {
    public: invalid_file_path(const char * path) : invalid_argument(format("file does not exist: {}", path)) {}
};

class invalid_degrees : public invalid_argument {
    public: invalid_degrees(int deg) : invalid_argument(format("degrees should be multiple of 90, got {}", deg)) {}
    public: invalid_degrees(const char * deg) : invalid_argument(format("degrees should be multiple of 90, got {}", deg)) {}
};

class invalid_coordinates : public invalid_argument {
    public: invalid_coordinates(const char * str) : invalid_argument(format("invalid coordinates: {}", str)) {}
    public: invalid_coordinates(vec2<int> a, vec2<int> b) : invalid_argument(format("invalid coordinates: {}, {}", a, b)) {}
};