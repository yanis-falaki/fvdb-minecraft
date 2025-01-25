/*
This script takes an (x, y, z) coordinate as input, and prints the nbt data of the chunk it's located in.\
This was written for educational purposes and being kept for future reference.
You can understand the essential NBT parsing logic by reading through this file.

Useful resources:
https://minecraft.fandom.com/wiki/Region_file_format
https://minecraft.fandom.com/wiki/Chunk_format
https://minecraft.fandom.com/wiki/NBT_format
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <bitset>
#include <zlib.h>
#include <vector>
#include <helpers.h>
#include <constants.h>

using namespace constants;

void exploreCompound(std::vector<uint8_t>::iterator& iterator);
void exploreList(std::vector<uint8_t>::iterator& iterator);

int main()
{
    int32_t x = 5;
    int32_t y = 122;
    int32_t z = -8;

    // Chunk to look for
    int chunkX = x >> 4;
    int chunkZ = z >> 4;

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

    std::vector<uint8_t> data = helpers::uncompress_chunk(compressed, size);

    helpers::dumpVectorToFile(data, "chunk.nbt");

    std::vector<uint8_t>::iterator iterator = data.begin(); // skip the first 3 bytes as it describes overarching compound tag.
    
    exploreCompound(iterator);

    inputFile.close();
    return 0;
}

void exploreCompound(std::vector<uint8_t>::iterator& iterator){
    while (true) {
        Tag tag = static_cast<Tag>(*iterator);

        if (tag == Tag::End){
            ++iterator;
            return;
        }

        uint16_t nameLength = (*(iterator + 1) << 8) | (*(iterator + 2));
        char nameArray[nameLength + 1];
        helpers::strcpy(iterator + 3, nameArray, nameLength);
        std::cout << "Tag: " << helpers::toStr(tag) << "\tName: " << nameArray << std::endl;

        iterator += 3 + nameLength;

        // iterator is at start of payload
        switch (tag) {

            case Tag::Byte:
            case Tag::Short:
            case Tag::Int:
            case Tag::Long:
            case Tag::Float:
            case Tag::Double:
                iterator += helpers::getPayloadLength(tag);
                break;

            case(Tag::Byte_Array): {
                uint32_t length = (*iterator << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | *(iterator + 3);
                iterator += 4 + length;
                break;
            }

            case(Tag::String): {
                uint16_t stringLength = (*iterator << 8) | (*(iterator + 1));
                char stringArray[stringLength + 1];
                helpers::strcpy(iterator+2, stringArray, stringLength);
                std::cout << stringArray << std::endl;

                iterator += 2 + stringLength;
                break;
            }

            case(Tag::List): {
                exploreList(iterator);
                break;
            }

            case (Tag::Compound): {
                exploreCompound(iterator);
                break;
            }

            case(Tag::Int_Array): {
                int size = (*(iterator) << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | (*(iterator + 3));
                iterator += 4 + size*4;
                break;
            }

            case(Tag::Long_Array): {
                int size = (*(iterator) << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | (*(iterator + 3));
                iterator += 4 + size*8;
                break;
            }
        }
    }
}

void exploreList(std::vector<uint8_t>::iterator& iterator) {
    uint8_t payloadTagLength =  helpers::getPayloadLength(*iterator);
    Tag list_tag = static_cast<Tag>(*iterator);

    std::cout << "List Tag: " << helpers::toStr(list_tag) << std::endl;
    uint32_t listLength = (*(iterator + 1) << 24) | (*(iterator + 2) << 16) | (*(iterator + 3) << 8) | (*(iterator + 4));

    iterator += 5 + (payloadTagLength * listLength);

    // If atypical list_tag, then the iterator will have only jumped 5 units as payloadTagLength will be zero
    if (list_tag == Tag::Compound) {
        for (int i = 0; i < listLength; ++i) exploreCompound(iterator);
    }
    else if (list_tag == Tag::String) {
        for (int i = 0; i < listLength; ++i) {
            iterator += 2 + ((*iterator << 8) | (*(iterator + 1))); // 2 bytes for length storage, + length bytes for string content. 
        }
    }
    else if (list_tag == Tag::List) {
        for (int i = 0; i < listLength; ++i) {
            exploreList(iterator);
        }
    }
}
