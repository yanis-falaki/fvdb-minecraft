#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <openvdb/openvdb.h>

#include <NBTParser.h>
#include <NBTVDB.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#include <nanovdb/util/IO.h>

int main() 
{
    int32_t regionX = 0;
    int32_t regionZ = 0;

    // -----------------
    openvdb::initialize();
    openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create(0);
    openvdb::Int32Grid::Accessor accessor = grid->getAccessor();
     // -----------------
    
    std::string regionFolderPath = std::format("{}/data/test_world/region", PROJECT_SOURCE_DIR);
    NBTParser::GlobalPalette globalPalette;

    NBTParser::VDB::populateVDBWithRegionFile(regionFolderPath, regionX, regionZ, *grid, globalPalette);
    
    // Writing grid to file as an ordinary VDB grid.
    grid->setName("RegionExample");
    openvdb::io::File file("region.vdb");
    openvdb::GridPtrVec grids;
    grids.push_back(grid);
    file.write(grids);
    file.close();

    // Writing grid to file as a NanoVDB grid.
    auto handle = nanovdb::tools::createNanoGrid(*grid);
    nanovdb::io::writeGrid("region.nvdb", handle);

    return 0;
}