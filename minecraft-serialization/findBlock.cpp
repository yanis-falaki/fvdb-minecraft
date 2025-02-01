#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <NBTParser.h>

void serializeChunk(NBTParser::SectionPack section, NBTParser::GlobalPalette globalPalette, int32_t xOffset, int32_t zOffset, int32_t x, int32_t y, int32_t z);

int main()
{
    int32_t x = 110;
    int32_t y = 62;
    int32_t z = 250;

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
    uint8_t* compressed = new uint8_t[size];
    inputFile.read(reinterpret_cast<char*>(compressed), size);

    uint32_t uncompressedSize;
    uint8_t* data = NBTParser::helpers::uncompress_chunk(compressed, size, uncompressedSize);

    delete[] compressed;

    //helpers::dumpArrayToFile(data, uncompressedSize, "chunk.nbt");

    NBTParser::SectionListPack sectionList = NBTParser::getSectionListPack(data, chunkX, chunkZ);

    int32_t sectionIndex = sectionList.getSectionIndexWithY(chunkY);
    if (sectionIndex == -1){
        std::cout << "Block being searched for does not exist!" << std::endl;
        return 1;
    }

    int32_t i_coords[NBTParser::SECTION_SIZE*sectionList.size()];
    int32_t j_coords[NBTParser::SECTION_SIZE*sectionList.size()];
    int32_t k_coords[NBTParser::SECTION_SIZE*sectionList.size()];
    int32_t palette_index[NBTParser::SECTION_SIZE*sectionList.size()];

    NBTParser::GlobalPalette globalPalette;

    NBTParser::sectionListToCoords(globalPalette, sectionList, i_coords, j_coords, k_coords, palette_index);

    uint32_t dataIndex = NBTParser::helpers::globalCoordsToSectionDataIndex(x, y, z);

    std::cout << "Block: " << globalPalette[palette_index[dataIndex + sectionIndex*NBTParser::SECTION_SIZE]] << "\t Index: " << palette_index[dataIndex + sectionIndex*NBTParser::SECTION_SIZE] <<  std::endl;
    std::cout << "Coords: " << i_coords[dataIndex + sectionIndex*NBTParser::SECTION_SIZE] << ", " << j_coords[dataIndex + sectionIndex*NBTParser::SECTION_SIZE] << ", " << k_coords[dataIndex + sectionIndex*NBTParser::SECTION_SIZE] << std::endl;

    serializeChunk(sectionList[sectionIndex], globalPalette, sectionList.xOffset, sectionList.zOffset, x, y, z);
    
    delete[] data;
    inputFile.close();
    return 0;
}

void serializeChunk(NBTParser::SectionPack section, NBTParser::GlobalPalette globalPalette, int32_t xOffset, int32_t zOffset, int32_t x, int32_t y, int32_t z) {
    // -----------------
    openvdb::initialize();
    openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create(0);
    openvdb::Int32Grid::Accessor accessor = grid->getAccessor();
     // -----------------

    NBTParser::populateVDBWithSection(globalPalette, section, xOffset, zOffset, accessor);

    grid->setName("ChunkExample");
    grid->setTransform(
        openvdb::math::Transform::createLinearTransform(4));
    grid->setGridClass(openvdb::GRID_FOG_VOLUME);

    std::cout << globalPalette[accessor.getValue(openvdb::Coord(x, y, z))] << std::endl;

    openvdb::io::File file("chunk.vdb");

    // Add the grid pointer to a container.
    openvdb::GridPtrVec grids;
    grids.push_back(grid);

    file.write(grids);
    file.close();
}
