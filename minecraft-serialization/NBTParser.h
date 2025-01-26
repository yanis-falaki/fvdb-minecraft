/*
This script takes an (x, y, z) coordinate as input, and prints the nbt data of the chunk it's located in.\
This was written for educational purposes and being kept for future reference.
You can understand the essential NBT parsing logic by reading through this file.

Useful resources:
https://minecraft.fandom.com/wiki/Region_file_format
https://minecraft.fandom.com/wiki/Chunk_format
https://minecraft.fandom.com/wiki/NBT_format
*/

#ifndef INCLUDED_NBT_PARSER
#define INCLUDED_NBT_PARSER

#include <iostream>
#include <cstdint>
#include <vector>
#include <helpers.h>
#include <cstring>

template<typename IterType>
bool exploreCompound(IterType& iterator);

template<typename IterType>
void exploreList(IterType& iterator);

template<typename IterType>
void getBlockFromSection(IterType& iterator, int32_t x, int32_t y, int32_t z);

template<typename IterType>
bool readSection(IterType& iterator, IterType& dataIterator, int32_t& y);

// --------------------------> Tags enum <--------------------------

enum class Tag {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    Byte_Array = 7,
    String = 8,
    List = 9,
    Compound = 10,
    Int_Array = 11,
    Long_Array = 12
};

// --------------------------> TagType <--------------------------

template<Tag T>
struct TagType;

template<>
struct TagType<Tag::Byte> {
    using Type = int8_t;
};

template<>
struct TagType<Tag::Short> {
    using Type = int16_t;
};

template<>
struct TagType<Tag::Int> {
    using Type = int32_t;
};

template<>
struct TagType<Tag::Long> {
    using Type = int64_t;
};

template<>
struct TagType<Tag::Float> {
    using Type = float;
};

template<>
struct TagType<Tag::Double> {
    using Type = double;
};

// --------------------------> toStr <--------------------------

// Function to convert Tag to string
inline const char* toStr(Tag tag) {
    switch (tag) {
        case Tag::End:           return "End";
        case Tag::Byte:          return "Byte";
        case Tag::Short:         return "Short";
        case Tag::Int:           return "Int";
        case Tag::Long:          return "Long";
        case Tag::Float:         return "Float";
        case Tag::Double:        return "Double";
        case Tag::Byte_Array:    return "Byte_Array";
        case Tag::String:        return "String";
        case Tag::List:          return "List";
        case Tag::Compound:      return "Compound";
        case Tag::Int_Array:     return "Int_Array";
        case Tag::Long_Array:    return "Long_Array";
        default:                 return "Unknown";
    }
}

// --------------------------> payloadLengthMap <--------------------------

// If the payload could have variable length or requires special preprocessing then length is set to 0
const uint8_t PAYLOAD_LENGTH_MAP[] = {
    1,   // End,
    1,   // Byte
    2,   // Short
    4,   // Int
    8,   // Long
    4,   // Float
    8,   // Double
    0,  // Byte_Array
    0,  // String
    0,  // List
    0,  // Compound
    0,  // Int_Array
    0   // Long_Array
};

// --------------------------> getPayloadLength <--------------------------

// Function to get payload length for a given Tag
inline uint8_t getPayloadLength(Tag tag) {
    return PAYLOAD_LENGTH_MAP[static_cast<int>(tag)];
}
inline uint8_t getPayloadLength(uint8_t tag) {
    return PAYLOAD_LENGTH_MAP[tag];
}

// --------------------------> parseTagAndName <--------------------------

struct TagAndName {
    bool isEnd;        // Indicates if the tag is an End tag.
    Tag tag;           // The current tag.
    std::string name;  // Parsed name as a string.
};

template <typename IteratorType>
TagAndName parseTagAndName(IteratorType& iterator) {
    TagAndName result;

    // Parse the tag
    result.tag = static_cast<Tag>(*iterator);
    if (result.tag == Tag::End) {
        ++iterator;  // Move the iterator forward
        result.isEnd = true;
        return result;
    }

    result.isEnd = false;

    // Parse the name length
    uint16_t nameLength = (*(iterator + 1) << 8) | (*(iterator + 2));

    // Parse the name
    char nameArray[nameLength + 1];
    helpers::strcpy(iterator + 3, nameArray, nameLength);
    result.name = nameArray;

    // Move the iterator past the parsed data
    iterator += 3 + nameLength;

    return result;
}

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
        auto tagAndName = parseTagAndName(iterator);

        if (tagAndName.isEnd) return false;

        //std::cout << "Tag: " << toStr(tagAndName.tag) << "\tName: " << tagAndName.name << std::endl;

        // iterator is at start of payload
        switch (tagAndName.tag) {

            case Tag::Byte: {
                auto value = readNum<Tag::Byte, decltype(iterator)>(iterator);
                iterator += getPayloadLength(Tag::Byte);
                break;
            }
            case Tag::Short: {
                auto value = readNum<Tag::Short, decltype(iterator)>(iterator);
                iterator += getPayloadLength(Tag::Short);
                break;
            }
            case Tag::Int: {
                auto value = readNum<Tag::Int, decltype(iterator)>(iterator);
                iterator += getPayloadLength(Tag::Int);
                break;
            }
            case Tag::Long: {
                auto value = readNum<Tag::Long, decltype(iterator)>(iterator);
                iterator += getPayloadLength(Tag::Long);
                break;
            }
            case Tag::Float: {
                auto value = readNum<Tag::Float, decltype(iterator)>(iterator);
                iterator += getPayloadLength(Tag::Float);
                break;
            }
            case Tag::Double: {
                auto value = readNum<Tag::Double, decltype(iterator)>(iterator);
                iterator += getPayloadLength(Tag::Double);
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
    uint8_t payloadTagLength =  getPayloadLength(*iterator);
    Tag list_tag = static_cast<Tag>(*iterator);

    //std::cout << "List Tag: " << toStr(list_tag) << std::endl;
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
        auto tagAndName = parseTagAndName(iterator);
        
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

#endif