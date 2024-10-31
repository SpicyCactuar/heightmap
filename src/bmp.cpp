#include <iostream>

unsigned char* loadBMP(const char* path, int& width, int& height) {
    std::cout << "Reading file: " << path << std::endl;

    // Data read from the header of the BMP file
    unsigned char header[54];

    // Open the file
    FILE* file = fopen(path, "rb");
    if (!file) {
        std::cout << path << " could not be opened. Are you in the right directory?" << std::endl;
        return nullptr;
    }

    // Read the header, i.e. the 54 first bytes
    // If less than 54 bytes are read, problem
    if (fread(header, 1, 54, file) != 54) {
        std::cout << "Not a correct BMP file" << std::endl;
        fclose(file);
        return nullptr;
    }
    // A BMP files always begins with "BM"
    if (header[0] != 'B' || header[1] != 'M') {
        std::cout << "Not a correct BMP file" << std::endl;
        fclose(file);
        return nullptr;
    }

    // Make sure this is a 24bpp file
    if (*(int*) &(header[0x1E]) != 0) {
        std::cout << "Not a correct BMP file" << std::endl;
        fclose(file);
        return nullptr;
    }
    if (*(int*) &(header[0x1C]) != 24) {
        std::cout << "Not a correct BMP file" << std::endl;
        fclose(file);
        return nullptr;
    }

    // Read the information about the image
    unsigned int dataPos = *(int*) &(header[0x0A]);
    unsigned int imageSize = *(int*) &(header[0x22]);
    width = *(int*) &(header[0x12]);
    height = *(int*) &(header[0x16]);

    // Some BMP files are malformed, guess missing information
    if (imageSize == 0) {
        imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
    }
    if (dataPos == 0) {
        dataPos = 54; // The BMP header is done that way
    }

    // Create a buffer
    const auto data = new unsigned char[imageSize];

    // Read the actual data from the file into the buffer
    fseek(file, dataPos, SEEK_SET);
    fread(data, 1, imageSize, file);

    // Everything is in memory now, the file can be closed.
    fclose(file);
    return data;
}
