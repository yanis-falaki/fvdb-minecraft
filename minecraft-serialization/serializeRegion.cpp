#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <NBTParser.h>

constexpr uint32_t MAX_CHUNKS = 1024;

inline void regionChunkIndexToGlobalChunkCoords(uint32_t index, uint32_t regionX, uint32_t regionZ, uint32_t& destChunkX, uint32_t& destChunkZ);

struct IJKContainer {
    int32_t* i;
    int32_t* j;
    int32_t* k;
    int32_t* paletteIndex;

    IJKContainer() {
        i, j, k, paletteIndex = nullptr;
    }

    IJKContainer(uint32_t numSections) {
        uint32_t arraySize = NBTParser::SECTION_SIZE*numSections;
        i = new int32_t[arraySize];
        j = new int32_t[arraySize];
        k = new int32_t[arraySize];
        paletteIndex = new int32_t[arraySize];
    }
};


int main() 
{
    int32_t regionX = 0;
    int32_t regionZ = 0;
    
    std::string region_file_path = std::format("{}/data/test_world/region/r.{}.{}.mca", PROJECT_SOURCE_DIR, regionX, regionZ);

    std::ifstream inputFile(region_file_path, std::ios::binary);

   if (!inputFile.is_open()) {
        std::cerr << "Error opening file " << region_file_path << std::endl;
        return 1;
    }

    uint8_t table_bytes[4];
    uint32_t offset; // Offset into file for compressed chunk
    uint8_t chunk_header[5];
    uint32_t chunkSize;
    uint32_t chunkX;
    uint32_t chunkZ;

    NBTParser::GlobalPalette globalPalette;

    std::vector<IJKContainer> chunkIJKs;

    for (uint32_t i = 0; i < MAX_CHUNKS; ++i) {
        uint32_t chunk_table_offset = i << 2; // every entry is 4 bytes
        inputFile.seekg(chunk_table_offset);

        inputFile.read(reinterpret_cast<char*>(table_bytes), 4);
        if (*(reinterpret_cast<uint32_t*>(table_bytes)) == 0) continue; // check that sector count and offset values are zero, if so then the chunk does not exist

        // Convert bytes to a big-endian left shifted by 12. (In order to seek to the 4KiB = 4096B offset)
        offset = (table_bytes[0] << 28) | (table_bytes[1] << 20) | table_bytes[2] << 12;

        // Seek to offset
        inputFile.seekg(offset, std::ios::beg);

        // Read byte count and compression format
        inputFile.read(reinterpret_cast<char*>(chunk_header), 5);

        // Check if the chunk is not compressed with zlib
        if (chunk_header[4] != 2) {
            std::cout << "Chunk: " << i << " not compressed with zlib... skipping." << std::endl;
            continue;
        }

        // Interpret size
        chunkSize = ((chunk_header[0] << 24) | (chunk_header[1] << 16) | (chunk_header[2] << 8) | chunk_header[3]) - 1;

        // Collect dataint
        uint8_t* compressed = new uint8_t[chunkSize];
        inputFile.read(reinterpret_cast<char*>(compressed), chunkSize);

        uint32_t uncompressedSize;
        uint8_t* data = NBTParser::helpers::uncompress_chunk(compressed, chunkSize, uncompressedSize);

        delete[] compressed;

        regionChunkIndexToGlobalChunkCoords(i, regionX, regionZ, chunkX, chunkZ); // assigns chunkX and chunkZ to global coords
        NBTParser::SectionListPack sectionList = NBTParser::getSectionListPack(data, chunkX, chunkZ);

        IJKContainer ijks(sectionList.size());

        NBTParser::sectionListToCoords(globalPalette, sectionList, ijks.i, ijks.j, ijks.k, ijks.paletteIndex);

        chunkIJKs.push_back(ijks);
    }

    return 0;
}

inline void regionChunkIndexToGlobalChunkCoords(uint32_t index, uint32_t regionX, uint32_t regionZ, uint32_t& destChunkX, uint32_t& destChunkZ) {
    destChunkX = (regionX & 31) + regionX;
    destChunkZ = (index >> 5) + regionZ;
    return;
}