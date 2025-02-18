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

void serializeWorldRegionsInSeries(NBTParser::GlobalPalette& globalPalette, std::string worldName, int32_t minimumSectionY);

int main() {
    openvdb::initialize();
    NBTParser::GlobalPalette globalPalette(std::format("{}/minecraft-serialization/block_list.txt", ROOT_DIR));

    const std::filesystem::path worlds{std::format("{}/data/raw_data/custom_saves/", ROOT_DIR)};

    for (auto const& world : std::filesystem::directory_iterator{worlds}) {
        serializeWorldRegionsInSeries(globalPalette, world.path().filename(), 0); // minimumSectionY is zero. (World also exists in negative coodinates but I'm only interested in the surface)
    }
}

// TODO make multithreaded
void serializeWorldRegionsInSeries(NBTParser::GlobalPalette& globalPalette, std::string worldName, int32_t minimumSectionY) {

    const std::filesystem::path regions{std::format("{}/data/raw_data/custom_saves/{}/region", ROOT_DIR, worldName)};

    openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create(0);
    openvdb::Int32Grid::Accessor accessor = grid->getAccessor();

    for (auto const& dir_entry : std::filesystem::directory_iterator{regions}) {
        grid->clear();
        accessor.clear();

        int32_t regionX;
        int32_t regionZ;
        NBTParser::helpers::parseRegionCoordinatesFromString(dir_entry.path().filename(), regionX, regionZ);

        // set regionX and regionZ parameter to 0, 0 since grid will only contain single region
        NBTParser::VDB::populateVDBWithRegionFile(dir_entry.path(), 0, 0, accessor, globalPalette, minimumSectionY);
        grid->pruneGrid(0);

        if (grid->activeVoxelCount() <= 0) continue; // empty region

        std::string worldRegionName = std::format("{}.{}.{}", worldName, regionX, regionZ);
        grid->setName(worldRegionName);

        //Writing grid to file as a NanoVDB grid.
        const auto handle = nanovdb::tools::createNanoGrid(*grid);
        nanovdb::io::writeGrid(std::format("{}/data/training_data/regions/{}.nvdb", ROOT_DIR, worldRegionName), handle, nanovdb::io::Codec::BLOSC);
    }
}