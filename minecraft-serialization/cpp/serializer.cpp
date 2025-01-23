#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>

int main()
{   
    int8_t x = 5;
    int8_t y = 122;
    int8_t z = -8;

    // Chunk to look for
    int chunkX = x >> 4;
    int chunkZ = z >> 4;

    std::cout << "chunkX: " << chunkX << "\tchunkZ: " << chunkZ << std::endl;

    // Find region; https://minecraft.fandom.com/wiki/Region_file_format
    int regionX = chunkX >> 5;
    int regionZ = chunkZ >> 5;

    std::cout << "regionX: " << regionX << "\tregionZ: " << regionZ << std::endl;

    // construct string to open region file
    std::stringstream filePath;
    filePath << "./test_world/region/r." << regionX << "." << regionZ << ".mca";

    // open region file
    std::ifstream inputFile(filePath.str(), std::ios::binary);

    // Check if the file was successfully opened
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file " << filePath.str() << std::endl;
        return 1;
    }
    
    // Find chunk_table_offset, this table then gives offset into file for chunk
    int chunk_table_offset = 4 * ((chunkX & 31) + (chunkZ & 31) * 32);

    // seek pointer to offset
    inputFile.seekg(chunk_table_offset, std::ios::beg);

    // Read 4 bytes, the first 3 are the offset, the last is sector count
    unsigned char bytes[4];
    inputFile.read(reinterpret_cast<char*>(bytes), 4);

    // Convert bytes to a big-endian left shifted by 12. (In order to seek to the 4KiB = 4096B offset)
    uint32_t offset = (bytes[0] << 28) | (bytes[1] << 20) | bytes[2] << 12;
    std::cout << "Offset: " << offset << std::endl;

    if ((offset | bytes[3]) == 0){
        std::cout << "Chunk does not exist" << std::endl;
        std::cout << "offset: " << offset << "\tsector count: " << bytes[3] << std::endl;
        std::cout << (offset | bytes[3]) << std::endl;
        return 1;
    }

    inputFile.close();
    return 0;
}