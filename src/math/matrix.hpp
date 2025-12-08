#pragma once

#include <format>
#include <string>
#include <vector>

template<typename T>
class matrix {
    unsigned int m, n;
    std::vector<T> elements;

    public:
        using reference = T&;

        matrix(unsigned int m, unsigned int n) {
            this->m = m;
            this->n = n;
            elements = std::vector<T>(m * n);
        }

        reference operator ()(unsigned int i, unsigned int j) {
            return elements.at(i * n + j);
        }

        reference at(unsigned int i, unsigned int j) {
            return elements.at(i * n + j);
        }

        void set_row(unsigned int i, std::span<T> row) {
            for (unsigned int j = 0; j < n; j++) {
                at(i, j) = row[j];
            } 
        }

        void set_column(unsigned int j, std::span<T> column) {
            for (unsigned int i = 0; i < m; i++) {
                at(i, j) = column[i];
            } 
        }

        T * data() {
            return elements.data();
        }

        size_t size() {
            return elements.size();
        }

        unsigned int rows() {
            return m;
        }

        unsigned int columns() {
            return n;
        }

        bool empty() {
            return size() == 0;
        }
};

#define matrix_formatter(T) template <>\
struct std::formatter<matrix<T>> : std::formatter<std::string> {\
  auto format(matrix<T> p, format_context& ctx) const {\
    if (p.empty()) {\
        return formatter<string>::format("[]", ctx);\
    }\
    string s = "[";\
    auto d = p.data();\
    for (int i = 0; i < p.size(); i++) {\
        s += std::format("{}", d[i]);\
        if ((i + 1) % p.columns() == 0 && i + 1 != p.size()) {\
            s += "\n ";\
        } else if (i + 1 == p.size()) {\
            s += "]";\
        } else {\
            s += ", ";\
        }\
    }\
    return formatter<string>::format(s, ctx);\
  }\
};

matrix_formatter(unsigned int);
matrix_formatter(int);
