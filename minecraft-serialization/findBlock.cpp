#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <NBTParser.h>

int bit_length(int n);

int main()
{
    int32_t x = -41;
    int32_t y = 104;
    int32_t z = 62;

    // Chunk to look for
    int chunkX = x >> 4;
    int chunkZ = z >> 4;
    int chunkY = y >> 4; // desired section_y

    std::cout << "chunkX: " << chunkX << "\tchunkZ: " << chunkZ << std::endl;

    // Find region; https://minecraft.fandom.com/wiki/Region_file_format
    int regionX = chunkX >> 5;
    int regionZ = chunkZ >> 5;

    std::cout << "regionX: " << regionX << "\tregionZ: " << regionZ << std::endl;

    // construct string to open region file
    std::stringstream filePath;
    filePath << PROJECT_SOURCE_DIR << "/data/test_world/region/r." << regionX << "." << regionZ << ".mca"; // PROJECT_SOURCE_DIR defined in CMakeLists.txt

    // open region file
    std::ifstream inputFile(filePath.str(), std::ios::binary);

    // Check if the file was successfully opened
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file " << filePath.str() << std::endl;
        return 1;
    }
    
    // Find chunk_table_offset, this table then gives offset into file for chunk
    int chunk_table_offset = ((chunkX & 31) + ((chunkZ & 31) << 5)) << 2;

    // seek pointer to offset
    inputFile.seekg(chunk_table_offset, std::ios::beg);

    // Read 4 bytes, the first 3 are the offset, the last is sector count
    unsigned char table_bytes[4];
    inputFile.read(reinterpret_cast<char*>(table_bytes), 4);

    // Convert bytes to a big-endian left shifted by 12. (In order to seek to the 4KiB = 4096B offset)
    uint32_t offset = (table_bytes[0] << 28) | (table_bytes[1] << 20) | table_bytes[2] << 12;
    std::cout << "Offset: " << offset << std::endl;

    if ((offset | table_bytes[3]) == 0){
        std::cout << "Chunk does not exist" << std::endl;
        std::cout << "offset: " << offset << "\tsector count: " << table_bytes[3] << std::endl;
        std::cout << (offset | table_bytes[3]) << std::endl;
        return 1;
    }

    // Seek to offset
    inputFile.seekg(offset, std::ios::beg);

    // Read byte count and compression format
    char chunk_header[5];
    inputFile.read(reinterpret_cast<char*>(chunk_header), 5);

    // Check if the chunk is not compressed with zlib
    if (chunk_header[4] != 2) std::cout << "Chunk at offset: " << offset << " not compressed with zlib... exiting." << std::endl;

    // Interpret size
    uint32_t size = ((chunk_header[0] << 24) | (chunk_header[1] << 16) | (chunk_header[2] << 8) | chunk_header[3]) - 1;

    // Collect data
    uint8_t compressed[size];
    inputFile.read(reinterpret_cast<char*>(compressed), size);

    uint32_t uncompressedSize;
    uint8_t* data = helpers::uncompress_chunk(compressed, size, uncompressedSize);

    helpers::dumpArrayToFile(data, uncompressedSize, "chunk.nbt");

    uint8_t* iterator = data;

    // skip everything until getting to sections, list of compounds
    //NBTParser::printNBTStructure(iterator);
    bool foundSections = NBTParser::findSectionsList(iterator);
    std::cout << "Found Sections: " << foundSections << std::endl;

    NBTParser::SectionListPack sectionList;
    NBTParser::sectionsList(iterator, sectionList);

    uint32_t section_index;
    for (uint32_t i = 0; i < sectionList.size(); ++i) {
        if (sectionList[i].y == chunkY){
            section_index = i;
            break;
        }
    }
    std::cout << "Section Index: " << section_index << std::endl;

    // get coords relative to section/chunk
    uint32_t localX = x & 15;
    uint32_t localY = y & 15;
    uint32_t localZ = z & 15;

    // calculate index within data array of section
    uint32_t dataIndex = localY * 256 + localZ*16 + localX;
    std::cout << "Data Index: " << dataIndex << std::endl;

    // calculate min number of bits to represent palette index
    uint32_t num_bits = bit_length(sectionList[section_index].blockStates.blockPalletePack.size());
    if (num_bits < 4) num_bits = 4;

    // make bitmask
    uint64_t bitmask = ((1ULL << num_bits) - 1);

    uint32_t indexes_per_element = 64 / num_bits;
    uint32_t last_state_elements = 4096 % indexes_per_element;
    if (last_state_elements == 0) last_state_elements = indexes_per_element;

    for (int i = 0; i < sectionList[section_index].blockStates.dataListLength; ++i) {
        uint64_t word = sectionList[section_index].blockStates.dataList[i];

        for (int j = 0; j < indexes_per_element; ++j) {
            uint32_t globalIndex = i * indexes_per_element + j;
            if (globalIndex == dataIndex) {
                uint32_t paletteIndex = (uint32_t)(word & bitmask);
                std::cout << "Block: " << sectionList[section_index].blockStates.blockPalletePack[paletteIndex].name << std::endl;
            }
            word = word >> num_bits;
        }
    }
    
    inputFile.close();
    return 0;
}

int bit_length(int n) {
    if (n == 0) return 0;
    
    // Convert to unsigned to deal with negative numbers in two's complement
    unsigned int un = n < 0 ? ~n + 1 : n;  // Two's complement for negative numbers
    
    int bits = 0;
    while (un) {
        un >>= 1; // Right shift by one bit
        bits++;
    }
    return bits;
}
