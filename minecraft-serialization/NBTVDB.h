#include <cstring>

#include <NBTParser.h>
#include <openvdb/openvdb.h>

namespace NBTParser::VDB {

// --------------------------> InsertSectionInVDBStrategy <--------------------------

struct InsertSectionInVDBStrategy : BaseUnpackSectionStrategy {
    inline void insert(uint32_t dataIndex, int32_t i, int32_t j, int32_t k, int32_t paletteIndex, auto& accessor) {
        if (paletteIndex == 0) return;
        accessor.setValue(openvdb::Coord(i, j, k), paletteIndex);
    }
};

// --------------------------> populateVDBWithSection <------------------------

void populateVDBWithSection(GlobalPalette& globalPalette, SectionPack& section, int32_t xOffset, int32_t zOffset, auto& accessor) {
    InsertSectionInVDBStrategy strategy;
    commonSectionUnpackingLogic(strategy, globalPalette, section, xOffset, zOffset, accessor);
}

// --------------------------> populateVDBWithSectionList <--------------------------

void populateVDBWithSectionList(GlobalPalette& globalPalette, SectionListPack& sectionList, auto& accessor) {
    InsertSectionInVDBStrategy strategy;
    uint32_t numSections = sectionList.size();

    // w << 12 = w*SECTION_SIZE
    for (uint32_t w = 0; w < numSections; ++w) {
        commonSectionUnpackingLogic(strategy, globalPalette, sectionList[w], sectionList.xOffset, sectionList.zOffset, accessor);
    }
}

// --------------------------> populateVDBWithRegionFile <--------------------------
void populateVDBWithRegionFile(std::string regionFilePath, int32_t regionX, int32_t regionZ, openvdb::Int32Grid::Accessor& accessor, GlobalPalette& globalPalette) {

    std::ifstream inputFile(regionFilePath, std::ios::binary);

   if (!inputFile.is_open()) {
        std::cerr << "Error opening file " << regionFilePath << std::endl;
        return;
    }

    uint8_t table_bytes[4];
    uint32_t offset; // Offset into file for compressed chunk
    uint8_t chunk_header[5];
    uint32_t chunkSize;
    int32_t chunkX;
    int32_t chunkZ;

    for (uint32_t i = 0; i < NBTParser::MAX_CHUNKS_IN_REGION; ++i) {
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

        std::cout << "Processing chunk " << i << " at (" << chunkX << ", " << chunkZ << ")\n";

        NBTParser::SectionListPack sectionList = NBTParser::getSectionListPack(data, chunkX, chunkZ);
        delete[] data;

        if (sectionList.size() <= 0) continue;

        NBTParser::VDB::populateVDBWithSectionList(globalPalette, sectionList, accessor);
    }

    inputFile.close();
}


}