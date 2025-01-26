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