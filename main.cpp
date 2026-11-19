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

void draw_dot(std::vector<uint8_t>& img, int w, int h, Vec2 p, uint8_t r, uint8_t g, uint8_t b) {
    int radius = 3;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= radius * radius) {
                put_pixel(img, w, h, p.x + dx, p.y + dy, r, g, b);
            }
        }
    }
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

struct State {
    std::vector<uint8_t>* img;
    int w;
    int h;
    Vec2 prev{0, 0};
    bool first{true};
    std::atomic<bool> active{false};
    std::atomic<bool> clicks{false};
};

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    State* st = reinterpret_cast<State*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (msg == WM_INPUT && st && st->active) {
        UINT size = 0;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size > 0) {
            std::vector<uint8_t> buf(size);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, buf.data(), &size, sizeof(RAWINPUTHEADER)) == size) {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buf.data());
                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    POINT p;
                    if (GetCursorPos(&p)) {
                        Vec2 cur = {p.x, p.y};
                        if (!st->first) {
                            draw_line(*st->img, st->w, st->h, st->prev, cur, 0, 0, 0);
                        }
                        st->prev = cur;
                        st->first = false;

                        if (st->clicks && (raw->data.mouse.usButtonFlags & (RI_MOUSE_LEFT_BUTTON_DOWN | RI_MOUSE_RIGHT_BUTTON_DOWN))) {
                            draw_dot(*st->img, st->w, st->h, cur, 255, 0, 0);
                        }
                    }
                }
            }
        }
        return 0;
    }

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

int main() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    std::vector<uint8_t> img(w * h * 3, 255); // change here if you want the background to be another colour
    State st{&img, w, h};
    std::atomic<DWORD> tid(0);

    std::thread worker([&]() {
        tid = GetCurrentThreadId();
        HINSTANCE inst = GetModuleHandle(nullptr);

        WNDCLASSA wc{};
        wc.lpfnWndProc = wnd_proc;
        wc.hInstance = inst;
        wc.lpszClassName = "raw_mouse_sink";
        RegisterClassA(&wc);

        HWND hwnd = CreateWindowExA(0, wc.lpszClassName, "", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, inst, nullptr);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&st));

        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    });

    std::cout << "mouse-tracker ready (/start, /stop, /click, /exit)\n";
    std::string cmd;

    while (std::cin >> cmd) {
        if (cmd == "/start") {
            std::fill(img.begin(), img.end(), 255);
            st.first = true;
            st.active = true;
            std::cout << "recording...\n";
        } else if (cmd == "/click") {
            st.clicks = !st.clicks;
            std::cout << "click markers: " << (st.clicks ? "on" : "off") << "\n";
        } else if (cmd == "/stop") {
            if (!st.active) {
                std::cout << "not recording right now\n";
                continue;
            }
            st.active = false;

            std::string dir = std::string(std::getenv("USERPROFILE")) + "\\Desktop\\";
            int n = 1;
            std::string out;
            do {
                out = dir + "mouse_pattern_" + std::to_string(n++) + ".png";
            } while (std::filesystem::exists(out));

            stbi_write_png(out.c_str(), w, h, 3, img.data(), w * 3);
            std::cout << "saved -> " << out << "\n";
        } else if (cmd == "/exit") {
            st.active = false;
            if (tid != 0) PostThreadMessage(tid, WM_QUIT, 0, 0);
            break;
        }
    }

    worker.join();
    return 0;
}