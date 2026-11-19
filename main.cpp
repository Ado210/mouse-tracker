#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <string>

void setPixel(std::vector<uint8_t>& canvas, int width, int height, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    // 1. Check: Ist das Kästchen überhaupt auf dem Papier? (sonst Absturz)
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    // 2. Finde das richtige Kästchen in der Liste (jedes Kästchen hat 3 Werte: R, G, B)
    int index = (y * width + x) * 3;

    // 3. Male das Kästchen an
    canvas[index]     = r; // Rot
    canvas[index + 1] = g; // Grün
    canvas[index + 2] = b; // Blau
}
    

int main() {
    int width = 1920;
    int height = 1080;

    
    std::vector<uint8_t> canvas(width * height * 3, 255); 

    setPixel(canvas, width, height, 960, 540, 0, 0, 0);
    // TODO: Setz mal testweise einen schwarzen Pixel (0, 0, 0) bei (x = 960, y = 540)
    // Formel für den Index im 1D-Array: int index = (y * width + x) * 3;

    
    std::string desktopPath = std::string(std::getenv("USERPROFILE")) + "\\Desktop\\mouse_pattern.png";

    stbi_write_png(desktopPath.c_str(), width, height, 3, canvas.data(), width * 3);
    std::cout << "Saved in: " << desktopPath << "\n";

    return 0;
}