#include <print>
#include <format>
#include <string_view>
#include "exceptions.hpp"
#include "format/bmp.hpp"

using namespace std;

void print_help() {
    println("Usage: bmpconvert <command> <input> [output]");
    println("Avaliable commands: -help, -info, -rotate, -inverse, -cut");
}

int main(int argc, char * argv[]) {
    if (argc < 2 || string_view(argv[1]) == "-help") {
        print_help();
        return 0;
    }

    std::string_view command_name(argv[1]);

    try {
        if (command_name == "-info") {
            if (argc < 3) {
                print_help();
                return 0;
            }
            Bitmap bmp(argv[2]);
            bmp.print_info();
        } else if (command_name == "-rotate") {
            if (argc < 5) {
                print_help();
                return 1;
            }

            int deg;
            try {
                deg = stoi(argv[2]);
            } catch (exception & e) {
                throw invalid_degrees(argv[2]);
            }
            Bitmap bmp(argv[3]);
            bmp.rotate(deg);
            bmp.write(argv[4]);
        } else if (command_name == "-inverse") {
            if (argc < 4) {
                print_help();
                return 1;
            }
            Bitmap bmp(argv[2]);
            bmp.inverse_colors();
            bmp.write(argv[3]);
        } else if (command_name == "-cut") {
            vec2 a {stoi(argv[2]), stoi(argv[3])};
            vec2 b {stoi(argv[4]), stoi(argv[5])};
            Bitmap bmp(argv[6]);
            bmp.cut(a, b);
            bmp.write(argv[7]);
        } else {
            print_help();
            return 1;
        }
    } catch (exception & e) {
        println("{}", e.what());
    }

    return 0;
}
