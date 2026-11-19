#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>

struct Vec2 {
    int x;
    int y;
};

void put_pixel(std::vector<uint8_t>& img, int w, int h, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    int idx = (y * w + x) * 3;
    img[idx]     = r;
    img[idx + 1] = g;
    img[idx + 2] = b;
}

void draw_line(std::vector<uint8_t>& img, int w, int h, Vec2 a, Vec2 b, uint8_t r, uint8_t g, uint8_t b_col) {
    int dx = std::abs(b.x - a.x);
    int dy = -std::abs(b.y - a.y);
    int sx = (a.x < b.x) ? 1 : -1;
    int sy = (a.y < b.y) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        put_pixel(img, w, h, a.x, a.y, r, g, b_col);
        if (a.x == b.x && a.y == b.y) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            a.x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            a.y += sy;
        }
    }
}

int main() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    std::vector<uint8_t> img(w * h * 3, 255); // change here if you want the background to be another colour
    std::atomic<bool> active(false);
    std::atomic<bool> quit(false);

    std::thread worker([&]() {
        Vec2 prev = {0, 0};
        bool first = true;

        while (!quit) {
            if (active) {
                POINT p;
                if (GetCursorPos(&p)) {
                    Vec2 cur = {p.x, p.y};
                    if (!first) {
                        draw_line(img, w, h, prev, cur, 0, 0, 0);
                    }
                    prev = cur;
                    first = false;
                }
            } else {
                first = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    std::cout << "mouse-tracker ready (/start, /stop, /exit)\n";
    std::string cmd;

    while (std::cin >> cmd) {
        if (cmd == "/start") {
            std::fill(img.begin(), img.end(), 255);
            active = true;
            std::cout << "recording...\n";
        } else if (cmd == "/stop") {
            if (!active) {
                std::cout << "not recording right now\n";
                continue;
            }
            active = false;

            std::string dir = std::string(std::getenv("USERPROFILE")) + "\\Desktop\\";
            int n = 1;
            std::string out;
            do {
                out = dir + "mouse_pattern_" + std::to_string(n++) + ".png";
            } while (std::filesystem::exists(out));

            stbi_write_png(out.c_str(), w, h, 3, img.data(), w * 3);
            std::cout << "saved -> " << out << "\n";
        } else if (cmd == "/exit") {
            active = false;
            quit = true;
            break;
        }
    }

    worker.join();
    return 0;
}