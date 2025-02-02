#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <format>
#include <filesystem>

#include <NBTParser.h>
#include <NBTVDB.h>
#include <openvdb/openvdb.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#include <nanovdb/util/IO.h>

inline void parseRegionCoordinates(const std::string& filename, int32_t& regionX, int32_t& regionZ);

int main() {
    openvdb::initialize();
    NBTParser::GlobalPalette globalPalette(std::format("{}/minecraft-serialization/block_list.txt", ROOT_DIR));
    const std::filesystem::path regions{std::format("{}/data/raw_data/custom_saves/1/region", ROOT_DIR)};

    for (auto const& dir_entry : std::filesystem::directory_iterator{regions}) {
        int32_t regionX;
        int32_t regionZ;
        parseRegionCoordinates(dir_entry.path().filename(), regionX, regionZ);

        openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create(0);
        openvdb::Int32Grid::Accessor accessor = grid->getAccessor();

        NBTParser::VDB::populateVDBWithRegionFile(dir_entry.path(), 0, 0, accessor, globalPalette);
        grid->pruneGrid(0);

        std::string worldRegionName = std::format("1.{}.{}", regionX, regionZ);
        grid->setName(worldRegionName);

        //Writing grid to file as a NanoVDB grid.
        auto handle = nanovdb::tools::createNanoGrid(*grid);
        nanovdb::io::writeGrid(std::format("{}/data/training_data/regions/{}.nvdb", ROOT_DIR, worldRegionName), handle);

        accessor.clear();
        grid->clear();
    }
}

inline void parseRegionCoordinates(const std::string& filename, int32_t& regionX, int32_t& regionZ) {
    // Navigate past the 'r.' prefix
    size_t startPos = 2;
    
    // Delineate coordinate boundaries via dot positions
    size_t firstDot = filename.find('.', startPos);
    size_t secondDot = filename.find('.', firstDot + 1);
    
    // Extract the coordinate substrings
    std::string xStr = filename.substr(startPos, firstDot - startPos);
    std::string zStr = filename.substr(firstDot + 1, secondDot - firstDot - 1);
    
    // Populate the reference parameters
    regionX = std::stoi(xStr);
    regionZ = std::stoi(zStr);
}