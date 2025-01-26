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
#include <cstring>

using namespace constants;

template<typename IterType>
bool exploreCompound(IterType& iterator);

template<typename IterType>
void exploreList(IterType& iterator);

template<typename IterType>
void getBlockFromSection(IterType& iterator, int32_t x, int32_t y, int32_t z);

template<typename IterType>
bool readSection(IterType& iterator, IterType& dataIterator, int32_t& y);

// --------------------------> Read Tag <--------------------------

template<Tag NumT, typename IterType>
inline typename TagType<NumT>::Type readNum(IterType& iterator) {
   static_assert((NumT == Tag::Byte) || (NumT == Tag::Short) || (NumT == Tag::Int) ||
                 (NumT == Tag::Long) || (NumT == Tag::Float) || (NumT == Tag::Double));

   using ResultType = typename TagType<NumT>::Type;

   if constexpr (NumT == Tag::Byte) {
       return *iterator;
   } else if constexpr (NumT == Tag::Short) {
       return static_cast<ResultType>((static_cast<uint16_t>(*iterator) << 8) | static_cast<uint16_t>(*(iterator + 1)));
   }
   else if constexpr (NumT == Tag::Int) {
       return static_cast<ResultType>(
           (static_cast<uint32_t>(*iterator) << 24) | 
           (static_cast<uint32_t>(*(iterator + 1)) << 16) | 
           (static_cast<uint32_t>(*(iterator + 2)) << 8) | 
           static_cast<uint32_t>(*(iterator + 3))
       );
   }
   else if constexpr (NumT == Tag::Long) {
       return static_cast<ResultType>(
           (static_cast<uint64_t>(*iterator) << 56) | 
           (static_cast<uint64_t>(*(iterator + 1)) << 48) | 
           (static_cast<uint64_t>(*(iterator + 2)) << 40) | 
           (static_cast<uint64_t>(*(iterator + 3)) << 32) |
           (static_cast<uint64_t>(*(iterator + 4)) << 24) | 
           (static_cast<uint64_t>(*(iterator + 5)) << 16) | 
           (static_cast<uint64_t>(*(iterator + 6)) << 8) | 
           static_cast<uint64_t>(*(iterator + 7))
       );
   }
   else if constexpr (NumT == Tag::Float) {
       return static_cast<ResultType>(
           (static_cast<uint32_t>(*iterator) << 24) | 
           (static_cast<uint32_t>(*(iterator + 1)) << 16) | 
           (static_cast<uint32_t>(*(iterator + 2)) << 8) | 
           static_cast<uint32_t>(*(iterator + 3))
       );
   }
   else if constexpr (NumT == Tag::Double) {
       return static_cast<ResultType>(
           (static_cast<uint64_t>(*iterator) << 56) | 
           (static_cast<uint64_t>(*(iterator + 1)) << 48) | 
           (static_cast<uint64_t>(*(iterator + 2)) << 40) | 
           (static_cast<uint64_t>(*(iterator + 3)) << 32) |
           (static_cast<uint64_t>(*(iterator + 4)) << 24) | 
           (static_cast<uint64_t>(*(iterator + 5)) << 16) | 
           (static_cast<uint64_t>(*(iterator + 6)) << 8) | 
           static_cast<uint64_t>(*(iterator + 7))
       );
   }
}

// --------------------------> Main Function <--------------------------

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

    uint32_t uncompressedSize;
    uint8_t* data = helpers::uncompress_chunk(compressed, size, uncompressedSize);

    uint8_t* iterator = data; // skip the first 3 bytes as it describes overarching compound tag.
    
    // skip everything until getting to sections, list of compounds
    bool foundSections = exploreCompound(iterator);
    std::cout << "Found Sections: " << foundSections << std::endl;

    getBlockFromSection(iterator, x, y, z);

    inputFile.close();
    return 0;
}

// --------------------------> exploreCompound <--------------------------

template<typename IterType>
bool exploreCompound(IterType& iterator){
    while (true) {
        auto tagAndName = helpers::parseTagAndName(iterator);

        if (tagAndName.isEnd) return false;

        //std::cout << "Tag: " << helpers::toStr(tagAndName.tag) << "\tName: " << tagAndName.name << std::endl;

        // iterator is at start of payload
        switch (tagAndName.tag) {

            case Tag::Byte: {
                auto value = readNum<Tag::Byte, decltype(iterator)>(iterator);
                iterator += helpers::getPayloadLength(tagAndName.tag);
                break;
            }
            case Tag::Short: {
                auto value = readNum<Tag::Short, decltype(iterator)>(iterator);
                iterator += helpers::getPayloadLength(tagAndName.tag);
                break;
            }
            case Tag::Int: {
                auto value = readNum<Tag::Int, decltype(iterator)>(iterator);
                iterator += helpers::getPayloadLength(tagAndName.tag);
                break;
            }
            case Tag::Long: {
                auto value = readNum<Tag::Long, decltype(iterator)>(iterator);
                iterator += helpers::getPayloadLength(tagAndName.tag);
                break;
            }
            case Tag::Float: {
                auto value = readNum<Tag::Float, decltype(iterator)>(iterator);
                iterator += helpers::getPayloadLength(tagAndName.tag);
                break;
            }
            case Tag::Double: {
                auto value = readNum<Tag::Double, decltype(iterator)>(iterator);
                iterator += helpers::getPayloadLength(tagAndName.tag);
                break;
            }

            case(Tag::Byte_Array): {
                uint32_t length = (*iterator << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | *(iterator + 3);
                iterator += 4 + length;
                break;
            }

            case(Tag::String): {
                uint16_t stringLength = (*iterator << 8) | (*(iterator + 1));
                //char stringArray[stringLength + 1];
                //helpers::strcpy(iterator+2, stringArray, stringLength);
                //std::cout << stringArray << std::endl;

                iterator += 2 + stringLength;
                break;
            }

            case(Tag::List): {
                if (static_cast<Tag>(*iterator) == Tag::Compound && tagAndName.name == "sections")
                    return true;
                exploreList(iterator);
                break;
            }

            case (Tag::Compound): {
                bool foundSections = exploreCompound(iterator);
                if (foundSections) return true;
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

// --------------------------> exploreList <--------------------------

template<typename IterType>
void exploreList(IterType& iterator) {
    uint8_t payloadTagLength =  helpers::getPayloadLength(*iterator);
    Tag list_tag = static_cast<Tag>(*iterator);

    //std::cout << "List Tag: " << helpers::toStr(list_tag) << std::endl;
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

template<typename IterType>
void getBlockFromSection(IterType& iterator, int32_t x, int32_t y, int32_t z) {
    uint32_t listLength = (*(iterator + 1) << 24) | (*(iterator + 2) << 16) | (*(iterator + 3) << 8) | (*(iterator + 4));
    int32_t section = y >> 4; // Y attribute we're looking for

    //readSection(iterator + 5, section);
}


/// @brief Returns 
/// @tparam IterType Iterator type
/// @param iterator Chunk Iterator
/// @param y Desired y level
/// @return 
template<typename IterType>
bool readSection(IterType& iterator, IterType& dataIterator, int32_t& y) {

    while(true) {
        auto tagAndName = helpers::parseTagAndName(iterator);
        
        std::cout << tagAndName.name << std::endl;


        if (tagAndName.name == "block_state") {

        }
        else if (tagAndName.name == "Y") {
        }
        else if (tagAndName.name == "biomes"){

        }
        else if (tagAndName.name == "BlockLight"){

        }
        else if (tagAndName.name == "SkyLight"){

        }
    }

    return false;
}