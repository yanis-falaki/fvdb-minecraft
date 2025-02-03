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

int main() {
    openvdb::initialize();
    NBTParser::GlobalPalette globalPalette(std::format("{}/minecraft-serialization/block_list.txt", ROOT_DIR));
    const std::filesystem::path regions{std::format("{}/data/raw_data/custom_saves/1/region", ROOT_DIR)};

    openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create(0);
    openvdb::Int32Grid::Accessor accessor = grid->getAccessor();

    for (auto const& dir_entry : std::filesystem::directory_iterator{regions}) {
        grid->clear();
        accessor.clear();

        int32_t regionX;
        int32_t regionZ;
        NBTParser::helpers::parseRegionCoordinatesFromString(dir_entry.path().filename(), regionX, regionZ);

        // set regionX and regionZ parameter to 0, 0 since we're only serializing a single region.
        NBTParser::VDB::populateVDBWithRegionFile(dir_entry.path(), 0, 0, accessor, globalPalette);
        grid->pruneGrid(0);

        std::string worldRegionName = std::format("1.{}.{}", regionX, regionZ);
        grid->setName(worldRegionName);

        //Writing grid to file as a NanoVDB grid.
        auto handle = nanovdb::tools::createNanoGrid(*grid);
        nanovdb::io::writeGrid(std::format("{}/data/training_data/regions/{}.nvdb", ROOT_DIR, worldRegionName), handle);
    }
}