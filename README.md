# fvdb-minecraft

## What's the goal of this repo
The goal of this repo is to perform deep learning using minecraft data, consisting of:

1. Game generated Worlds, Chunks & Regions
2. Custom Maps/Worlds
3. Minecraft Schematics

The reason for which is that Minecraft UGC seems to be an overlooked data source, which could be benificial to the development of large-scale 3D worlds.

There are tens to hundreds of thousands of towns and cities that users have built and released publicly for anyone to use. It would seem plausible that using this data we can train generative model to build novel worlds.

## What does this repo currently include?
The main contribution of this repo thus far is an extremely fast (and only existing) implementation of a minecraft world/region/chunk to nvdb serializer. The fast parser itself is also usable as a standalone library, it doesn't necessarily have to serialize to nvdb files.

The secondary part is an example of using a SparseUNet built with fvdb in order to predict what block each voxel in an nvdb grid belongs to, given occupancy information.

## Directory Structure
 * [data/](./data)
    * [processing/](./data/processing/)
        * [serialize_world_chunks.cpp](./data/processing/serialize_world_chunks.cpp) Script which serializes all chunks in all worlds and stores them as nvdbs in data/training_data/chunks/
        * [serialize_world_regions.cpp](./data/processing/serialize_world_regions.cpp) Script which serializes all regions in all worlds and stores them as nvdbs in data/training_data/regions/
    * [raw_data/](./data/raw_data/)
        * [custom_saves/](./data/raw_data/custom_saves/) Worlds (saves) generated within minecraft then moved here
        * [schematics/](./data/raw_data/schematics/) Minecraft schematics downloaded off the web
    * [training_data/](./data/training_data/)
        * [chunks/](./data/training_data/chunks/) Chunks saved as nvdbs. Name format: world.globalChunkX.globalChunkZ.nvdb
        * [regions/](./data/training_data/regions/) Regions saved as nvdbs. Name format: world.regionX.regionZ.nvdb
 * [minecraft-serialization/](./minecraft-serialization) Contains a library for parsing and serializing minecraft worlds.
    * [archive/](./minecraft-serialization/archive/) Old attempts at serialization that are being kept for reference. Using prebuilt libraries that were very slow.
    * [examples/](./minecraft-serialization/examples/) Some example usages of NBTParser.h
    * [helpers.h](./minecraft-serialization/helpers.h) Helper functions used within the library
    * [NBTParser.h](./minecraft-serialization/NBTParser.h) The main Named Binary Tag (NBT) parsing logic. NBTs are minecraft's custom data format for world data.
    * [NBTVDB.h](./minecraft-serialization/NBTVDB.h) An extension to NBTParser.h which is used to serialize worlds into OpenVDB grids.
 * [test.ipynb](./test.ipynb) Notebook which trains a SparseUNet to predict block type per voxel given grid occupancy.
 * [UNet.py](./UNet.py) Contains the implementation for SparseUNet.
 * [utils.py](./utils.py) Contains some utility/helper functions for test.ipynb