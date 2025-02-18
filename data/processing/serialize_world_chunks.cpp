#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <format>
#include <filesystem>
#include <thread>

#include <NBTParser.h>
#include <NBTVDB.h>
#include <openvdb/openvdb.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#include <nanovdb/util/IO.h>

void serializeRegionAsChunks(NBTParser::GlobalPalette& globalPalette, std::string worldName, int32_t minimumSectionY);

int main() {
    openvdb::initialize();
    NBTParser::GlobalPalette globalPalette(std::format("{}/minecraft-serialization/block_list.txt", ROOT_DIR));

    const std::filesystem::path worlds{std::format("{}/data/raw_data/custom_saves/", ROOT_DIR)};

    for (auto const& world : std::filesystem::directory_iterator{worlds}) {
        serializeRegionAsChunks(globalPalette, world.path().filename(), 0); // minimumSectionY is zero. (World also exists in negative coodinates but I'm only interested in the surface)
    }
}

// TODO make multithreaded
void serializeRegionAsChunks(NBTParser::GlobalPalette& globalPalette, std::string worldName, int32_t minimumSectionY) {

    const std::filesystem::path regions{std::format("{}/data/raw_data/custom_saves/{}/region", ROOT_DIR, worldName)};

    openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create(0);
    openvdb::Int32Grid::Accessor accessor = grid->getAccessor();

    for (auto const& dir_entry : std::filesystem::directory_iterator{regions}) {
        int32_t regionX;
        int32_t regionZ;
        NBTParser::helpers::parseRegionCoordinatesFromString(dir_entry.path().filename(), regionX, regionZ);

        // open region file
        std::ifstream inputFile(dir_entry.path(), std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "Error opening file " << dir_entry.path() << std::endl;
            return;
        }

        // serialize individual chunks in region file
        serializeChunks(inputFile, globalPalette, worldName, minimumSectionY, regionX, regionZ, grid, accessor);

        // close region file
        inputFile.close();
    }
}

void serializeChunks(auto& inputFile, NBTParser::GlobalPalette& globalPalette, std::string worldName, int32_t minimumSectionY,
                     int32_t regionX, int32_t regionZ, openvdb::Int32Grid::Ptr grid, openvdb::Int32Grid::Accessor accessor) {

    uint8_t table_bytes[4];
    uint32_t offset; // Offset into file for compressed chunk
    uint8_t chunk_header[5];
    uint32_t chunkSize;
    int32_t chunkX;
    int32_t chunkZ;

    for (uint32_t i = 0; i < NBTParser::MAX_CHUNKS_IN_REGION; ++i) {
        grid->clear();
        accessor.clear();

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

        NBTParser::helpers::regionChunkIndexToGlobalChunkCoords(i, regionX, regionZ, chunkX, chunkZ); // assigns chunkX and chunkZ to global coords

        NBTParser::SectionListPack sectionList = NBTParser::getSectionListPack(data, chunkX, chunkZ);
        delete[] data;

        if (sectionList.size() <= 0) continue;

        NBTParser::VDB::populateVDBWithSectionList(globalPalette, sectionList, accessor, minimumSectionY, true);
        grid->pruneGrid(0);

        if (grid->activeVoxelCount() <= 0) continue; // empty chunk

        std::string worldChunkName = std::format("{}.{}.{}", worldName, chunkX, chunkZ);
        grid->setName(worldChunkName);

        //Writing grid to file as a NanoVDB grid.
        const auto handle = nanovdb::tools::createNanoGrid(*grid);
        nanovdb::io::writeGrid(std::format("{}/data/training_data/chunks/{}.nvdb", ROOT_DIR, worldChunkName), handle, nanovdb::io::Codec::BLOSC);
    }
}