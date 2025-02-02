/*
This script implements the main NBT parsing logic for a chunk within a region file.

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
#include <unordered_map>
#include <fstream>
#include <format>

namespace NBTParser {

// --------------------------> constant values <--------------------------
static constexpr size_t SECTION_SIZE = 4096;
static constexpr size_t SECTION_LENGTH = 16;
static constexpr size_t MAX_CHUNKS_IN_REGION = 1024;

// --------------------------> Parameter Pack Forward Declarations <--------------------------

struct BlockStatesPack;
struct SectionPack;
struct PaletteListPack;

// --------------------------> Compound Strategy Forward Declarations <--------------------------

bool findSectionsList(uint8_t*& iterator);
void printNBTStructure(uint8_t*& iterator);
inline bool skipNBTStructure(uint8_t*& iterator);
inline void blockStatesCompound(uint8_t*& iterator, BlockStatesPack& blockStatesPack);

// --------------------------> List Strategy Forward Declarations <--------------------------

inline void printList(uint8_t*& iterator);
inline void skipList(uint8_t*& iterator);
inline void sectionPalleteList(uint8_t*& iterator, PaletteListPack& blockPalettePack);

// --------------------------> GlobalPalette <--------------------------

struct GlobalPalette {
    std::vector<std::string> indexToStringVector;
    std::unordered_map<std::string, uint32_t> nameToIndexMap;

    GlobalPalette(std::string block_list_file_path) {
        std::ifstream infile(block_list_file_path);
        int i = 0;
        for (std::string line; std::getline(infile, line); ) 
        {
            nameToIndexMap[line] = i;
            indexToStringVector.push_back(line);
            ++i;
        }
    }

    // Numeric index operator
    const std::string& operator[](uint32_t index) const {
        return indexToStringVector[index];
    }

    // String lookup operator
    uint32_t operator[](const std::string& name) const {
        return nameToIndexMap.at(name);
    }

    uint32_t size() { return indexToStringVector.size(); }
};

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

// --------------------------> readNum <--------------------------

template<Tag NumT>
inline typename TagType<NumT>::Type readNum(auto&& iterator) {
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

// --------------------------> Parameter Packs <--------------------------

struct PalettePack {
    std::string name;
    PalettePack() = default;
};

struct PaletteListPack {
    std::vector<PalettePack> palette;
    
    PalettePack& operator[](size_t index) {
        return palette[index];
    }
    const PalettePack& operator[](size_t index) const {
        return palette[index];
    }
    PalettePack* operator+(size_t offset) {
        return palette.data() + offset;
    }
    const PalettePack* operator+(size_t offset) const {
        return palette.data() + offset;
    }
    PalettePack* begin() { return palette.data(); }
    PalettePack* end() { return palette.data() + palette.size(); }
    const PalettePack* begin() const { return palette.data(); }
    const PalettePack* end() const { return palette.data() + palette.size(); }

    size_t size() const { return palette.size(); }

    PaletteListPack() = default;
};

struct BlockStatesPack {
    uint64_t* dataList;
    uint32_t dataListLength;
    PaletteListPack palleteList;
    BlockStatesPack() = default;
};

struct SectionPack {
    BlockStatesPack blockStates;
    int32_t yOffset;
    int32_t y;
    SectionPack() = default;
};

// Section List AKA Chunk
struct SectionListPack {
    std::vector<SectionPack> sections;
    int32_t xOffset;
    int32_t zOffset;
    int32_t x;
    int32_t z;

    SectionListPack(int32_t chunkX, int32_t chunkZ) {
        xOffset = chunkX  << 4;
        zOffset = chunkZ << 4;
        x = chunkX;
        z = chunkZ;
    }
    
    SectionPack& operator[](size_t index) {
        return sections[index];
    }
    const SectionPack& operator[](size_t index) const {
        return sections[index];
    }
    SectionPack* operator+(size_t offset) {
        return sections.data() + offset;
    }
    const SectionPack* operator+(size_t offset) const {
        return sections.data() + offset;
    }
    SectionPack* begin() { return sections.data(); }
    SectionPack* end() { return sections.data() + sections.size(); }
    const SectionPack* begin() const { return sections.data(); }
    const SectionPack* end() const { return sections.data() + sections.size(); }

    inline bool getSectionWithY(SectionPack* dest, int32_t chunkY) {
        uint32_t section_index;
        for (uint32_t i = 0; i < sections.size(); ++i) {
            if (sections[i].y == chunkY){
                *dest = sections[i];
                return true;
            }
        }
        return false;
    }

    inline int32_t getSectionIndexWithY(int32_t chunkY) {
        uint32_t section_index;
        for (uint32_t i = 0; i < sections.size(); ++i) {
            if (sections[i].y == chunkY){
                return i;
            }
        }
        return -1;
    }

    size_t size() const { return sections.size(); }

    SectionListPack() = default;
};

// --------------------------> parseNBTStructure (main loop) <--------------------------

template<typename Strategy, typename... OptionalParamPack>
bool parseNBTStructure(uint8_t*& iterator, Strategy& strategy, OptionalParamPack&... optionalParamPack) {
    while (true) {
        auto tagAndName = parseTagAndName(iterator);
        strategy.preamble(iterator, tagAndName, optionalParamPack...); // optional printing
        if (tagAndName.isEnd) return false;

        switch (tagAndName.tag) {
            case Tag::Byte: 
                strategy.template handleByteTag(iterator, tagAndName, optionalParamPack...);
                iterator += getPayloadLength(Tag::Byte);
                break;
            case Tag::Short: 
                strategy.template handleNumericTag<Tag::Short>(iterator, tagAndName, optionalParamPack...);
                iterator += getPayloadLength(Tag::Short);
                break;
            case Tag::Int: 
                strategy.template handleNumericTag<Tag::Int>(iterator, tagAndName, optionalParamPack...);
                iterator += getPayloadLength(Tag::Int);
                break;
            case Tag::Long: 
                strategy.template handleNumericTag<Tag::Long>(iterator, tagAndName, optionalParamPack...);
                iterator += getPayloadLength(Tag::Long);
                break;
            case Tag::Float: 
                strategy.template handleNumericTag<Tag::Float>(iterator, tagAndName, optionalParamPack...);
                iterator += getPayloadLength(Tag::Float);
                break;
            case Tag::Double: 
                strategy.template handleNumericTag<Tag::Double>(iterator, tagAndName, optionalParamPack...);
                iterator += getPayloadLength(Tag::Double);
                break;

            case Tag::Byte_Array: {
                uint32_t length = (*iterator << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | *(iterator + 3);
                strategy.handleByteArray(iterator, tagAndName, length, optionalParamPack...);
                break;
            }

            case Tag::String: {
                uint16_t stringLength = (*iterator << 8) | (*(iterator + 1));
                strategy.handleString(iterator, tagAndName, stringLength, optionalParamPack...);
                iterator += 2 + stringLength;
                break;
            }

            case Tag::List: {
                if (strategy.handleList(iterator, tagAndName, optionalParamPack...))
                    return true;
                break;
            }

            case Tag::Compound: {
                if (strategy.handleCompound(iterator, tagAndName, optionalParamPack...))
                    return true;
                break;
            }

            case Tag::Int_Array: {
                int32_t length = readNum<Tag::Int>(iterator);
                strategy.handleIntArray(iterator, tagAndName, length, optionalParamPack...);
                break;
            }

            case Tag::Long_Array: {
                int32_t length = (*(iterator) << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | (*(iterator + 3));
                strategy.handleLongArray(iterator, tagAndName, length, optionalParamPack...);
                break;
            }
        }
    }
}

// --------------------------> BaseNBTStrategy <--------------------------

template<typename... OptionalParamPack>
struct BaseNBTStrategy {
    inline void preamble(uint8_t*& iterator, const auto& tagAndName, OptionalParamPack&... optionalParamPack) {}
    inline void handleByteTag(uint8_t*& iterator, const auto& tagAndName, OptionalParamPack&... optionalParamPack) {}
    template<Tag NumTag>
    inline void handleNumericTag(uint8_t*& iterator, const auto& tagAndName, OptionalParamPack&... optionalParamPack) {}
    inline void handleByteArray(uint8_t*& iterator, const auto& tagAndName, uint32_t length, OptionalParamPack&... optionalParamPack) {
        iterator += 4 + length;
    }
    inline void handleString(uint8_t*& iterator, const auto& tagAndName, uint16_t stringLength, OptionalParamPack&... optionalParamPack) {}
    inline bool handleList(uint8_t*& iterator, const auto& tagAndName, OptionalParamPack&... optionalParamPack) {
        skipList(iterator);
        return false;
    }
    inline bool handleCompound(uint8_t*& iterator, const auto& tagAndName, OptionalParamPack&... optionalParamPack) {
        return skipNBTStructure(iterator);
    }
    inline void handleLongArray(uint8_t*& iterator, const auto& tagAndName, int32_t length, OptionalParamPack&... optionalParamPack) {
        iterator += 4 + length*8;
    }
    inline void handleIntArray(uint8_t*& iterator, const auto& tagAndName, int32_t length, OptionalParamPack&... optionalParamPack) {
        iterator += 4 + length*4;
    }
};

// --------------------------> FindSectionsListStrategy <--------------------------

struct FindSectionsListStrategy : BaseNBTStrategy<> {
    inline bool handleList(uint8_t*& iterator, const auto& tagAndName) {
        if (tagAndName.name == "sections") return true;
        skipList(iterator);
        return false;
    }
};

// --------------------------> PrintNBTStructureStrategy <--------------------------

struct PrintNBTStructureStrategy : BaseNBTStrategy<> {
    inline void preamble(uint8_t*& iterator, const auto& tagAndName) {
        std::cout << "Tag: " << toStr(tagAndName.tag) << "\tName: " << tagAndName.name << std::endl;
    }

    inline void handleByteTag(uint8_t*& iterator, const auto& tagAndName) {
        auto value = readNum<Tag::Byte>(iterator);
        std::cout << "Value: " << static_cast<int>(value) << std::endl;
    }

    template<Tag NumTag>
    inline void handleNumericTag(uint8_t*& iterator, const auto& tagAndName) {
        auto value = readNum<NumTag>(iterator);
        std::cout << "Value: " << value << std::endl;
    }

    inline void handleByteArray(uint8_t*& iterator, const auto& tagAndName, uint32_t length) {
        iterator += 4;
        for (int i = 0; i < length; ++i) {
            std::cout << (short)(*(iterator));
            iterator += 1;
        }
        std::cout << std::endl;
    }

    inline void handleString(uint8_t*& iterator, const auto& tagAndName, uint16_t stringLength) {
        char stringArray[stringLength + 1];
        helpers::strcpy(iterator+2, stringArray, stringLength);
        std::cout << stringArray << std::endl;
    }

    inline bool handleList(uint8_t*& iterator, const auto& tagAndName) {
        printList(iterator);
        return false;
    }

    inline bool handleCompound(uint8_t*& iterator, const auto& tagAndName) {
        printNBTStructure(iterator);
        return false;
    }
};

// --------------------------> SectionCompoundStrategy <--------------------------

struct SectionCompoundStrategy : BaseNBTStrategy<SectionPack> {
    inline void handleByteTag(uint8_t*& iterator, const auto& tagAndName, SectionPack& sectionPack) {
        sectionPack.y = readNum<Tag::Byte>(iterator);
        sectionPack.yOffset = sectionPack.y << 4;
    }

    inline bool handleCompound(uint8_t*& iterator, const auto& tagAndName, SectionPack& sectionPack) {
        if (tagAndName.name == "block_states") blockStatesCompound(iterator, sectionPack.blockStates);
        else skipNBTStructure(iterator); 
        return false;
    }
};

// --------------------------> BlockStatesCompoundStrategy <--------------------------

struct BlockStatesCompoundStrategy : BaseNBTStrategy<BlockStatesPack> {
    inline void handleLongArray(uint8_t*& iterator, const auto& tagAndName, int32_t length, BlockStatesPack& blockStatesPack) {
        iterator += 4;
        blockStatesPack.dataList = new uint64_t[length];
        for (int i = 0; i < length; ++i) {
            blockStatesPack.dataList[i] = readNum<Tag::Long>(iterator);
            iterator += 8;
        }
        blockStatesPack.dataListLength = length;
    }

    inline bool handleList(uint8_t*& iterator, const auto& tagAndName, BlockStatesPack& blockStatesPack) {
        sectionPalleteList(iterator, blockStatesPack.palleteList);
        return false;
    }
};

// --------------------------> PaletteCompoundStrategy <--------------------------

struct PalleteCompoundStrategy : BaseNBTStrategy<PalettePack> {
    inline void handleString(uint8_t*& iterator, const auto& tagAndName, uint16_t stringLength, PalettePack& palettePack) {
        palettePack.name.assign(reinterpret_cast<char*>(iterator+2), stringLength);
    }
};


// --------------------------> Strategy Wrappers <--------------------------
inline bool findSectionsList(uint8_t*& iterator) {
    iterator += 3; // skipping overarching compound tag so that we can use skipNBTStructure in FindSectionsListStrategy::handleCompound.
                   // otherwise, we'd need to do comparison checks within every nested list within a compound tag for tagAndName.name == "sections",
                   // despite knowing it should be located at the top level.
    FindSectionsListStrategy strategy;
    return parseNBTStructure(iterator, strategy);
}

inline bool skipNBTStructure(uint8_t*& iterator) {
    BaseNBTStrategy strategy;
    return parseNBTStructure(iterator, strategy);
}

inline void printNBTStructure(uint8_t*& iterator) {
    PrintNBTStructureStrategy strategy;
    parseNBTStructure(iterator, strategy);
}

inline void sectionCompound(uint8_t*& iterator, SectionPack& sectionPack) {
    SectionCompoundStrategy strategy;
    parseNBTStructure(iterator, strategy, sectionPack);
}

inline void blockStatesCompound(uint8_t*& iterator, BlockStatesPack& blockStatesPack) {
    BlockStatesCompoundStrategy strategy;
    parseNBTStructure(iterator, strategy, blockStatesPack);
}

inline void paletteCompound(uint8_t*& iterator, PalettePack& palettePack) {
    PalleteCompoundStrategy strategy;
    parseNBTStructure(iterator, strategy, palettePack);
}

// --------------------------> exploreList <--------------------------

template<typename ListStrategy, typename... OptionalParamPack>
void exploreList(uint8_t*& iterator, ListStrategy listStrategy, OptionalParamPack&... optionalParamPack) {
    uint8_t payloadTagLength =  getPayloadLength(*iterator);
    Tag list_tag = static_cast<Tag>(*iterator);
    int32_t listLength = readNum<Tag::Int>(iterator+1);

    iterator += 5 + (payloadTagLength * listLength);

    listStrategy.preamble(list_tag, listLength, optionalParamPack...);

    if (list_tag == Tag::End) return;

    // If atypical list_tag, then the iterator will have only jumped 5 units as payloadTagLength will be zero
    if (list_tag == Tag::Compound) {
        listStrategy.handleCompound(iterator, listLength, optionalParamPack...);
    }
    else if (list_tag == Tag::String) {
        listStrategy.handleString(iterator, listLength, optionalParamPack...);
    }
    else if (list_tag == Tag::List) {
        listStrategy.handleList(iterator, listLength, optionalParamPack...);
    }
    else if (list_tag == Tag::Int_Array) {
        listStrategy.handleIntArray(iterator, listLength, optionalParamPack...);
    }
    else if (list_tag == Tag::Long_Array) {
        listStrategy.handleLongArray(iterator, listLength, optionalParamPack...);
    }
}

// --------------------------> List Strategies <--------------------------

template<typename... Args>
struct BaseListStrategy {
    inline void preamble(Tag list_tag, int32_t listLength, Args&... args) {}

    inline void handleCompound(uint8_t*& iterator, uint32_t listLength, Args&... args) {
        for (int i = 0; i < listLength; ++i) skipNBTStructure(iterator);
    }

    inline void handleString(uint8_t*& iterator, uint32_t listLength, Args&... args) {
        for (int i = 0; i < listLength; ++i) {
            int32_t stringLength = (*iterator << 8) | (*(iterator + 1));
            iterator += 2 + stringLength;
        }
    }

    inline void handleList(uint8_t*& iterator, uint32_t listLength, Args&... args) {
        for (int i = 0; i < listLength; ++i) skipList(iterator);
    }

    inline void handleIntArray(uint8_t*& iterator, uint32_t listLength, Args&... args) {
        int32_t currentArrayLength;
        for (int i = 0; i < listLength; ++i) {
            currentArrayLength = readNum<Tag::Int>(iterator);
            iterator += 4 + currentArrayLength*4;
        }
    }

    inline void handleLongArray(uint8_t*& iterator, uint32_t listLength, Args&... args) {
        int32_t currentArrayLength;
        for (int i = 0; i < listLength; ++i) {
            currentArrayLength = readNum<Tag::Int>(iterator);
            iterator += 4 + currentArrayLength*8;
        }
    }
};

struct PrintListStrategy : BaseListStrategy<> {
    inline void preamble(Tag list_tag, int32_t listLength) {
        if (list_tag == Tag::End) {
            std::cout << "Empty List of size: " << listLength << std::endl;
            return;
        }
        std::cout << toStr(list_tag) << " List of size: " << listLength << std::endl;
    }

    inline void handleCompound(uint8_t*& iterator, uint32_t listLength) {
        for (int i = 0; i < listLength; ++i) printNBTStructure(iterator);
    }

    inline void handleString(uint8_t*& iterator, uint32_t listLength) {
        for (int i = 0; i < listLength; ++i) {
            int32_t stringLength = (*iterator << 8) | (*(iterator + 1));

            char stringArray[stringLength + 1];
            helpers::strcpy(iterator+2, stringArray, stringLength);
            std::cout << stringArray << std::endl;

            iterator += 2 + stringLength;
        }
    }

    inline void handleList(uint8_t*& iterator, uint32_t listLength) {
        for (int i = 0; i < listLength; ++i) printList(iterator);
    }
};

struct SectionPaletteStrategy : BaseListStrategy<PaletteListPack> {
    inline void handleCompound(uint8_t*& iterator, uint32_t listLength, PaletteListPack& blockPalletePack) {
        blockPalletePack.palette.resize(listLength);
        for (int i = 0; i < listLength; ++i)
            paletteCompound(iterator, blockPalletePack.palette[i]);
    }
};

struct SectionListStrategy : BaseListStrategy<SectionListPack> {
    inline void handleCompound(uint8_t*& iterator, uint32_t listLength, SectionListPack& sectionListPack) {
        sectionListPack.sections.resize(listLength);
        for (int i = 0; i < listLength; ++i)
            sectionCompound(iterator, sectionListPack.sections[i]);
    }
};

// --------------------------> List Strategy Wrappers <--------------------------

inline void printList(uint8_t*& iterator) {
    PrintListStrategy listStrategy;
    exploreList(iterator, listStrategy);
}

inline void skipList(uint8_t*& iterator) {
    BaseListStrategy listStrategy;
    exploreList(iterator, listStrategy);
}

inline void sectionPalleteList(uint8_t*& iterator, PaletteListPack& blockPalettePack) {
    SectionPaletteStrategy listStrategy;
    exploreList(iterator, listStrategy, blockPalettePack);
}

inline void sectionsList(uint8_t*& iterator, SectionListPack& sectionsList) {
    SectionListStrategy listStrategy;
    exploreList(iterator, listStrategy, sectionsList);
}

// --------------------------> getSectionListPack <--------------------------

// 'iterator' is passed by value, so changes to it in this function
// do not affect the original position of the iterator passed by the caller.
SectionListPack getSectionListPack(uint8_t* localIterator, int32_t chunkX, int32_t chunkZ) {
    bool foundSections = findSectionsList(localIterator);
    SectionListPack sectionList(chunkX, chunkZ);
    sectionsList(localIterator, sectionList);
    return sectionList;
}

// --------------------------> commonSectionUnpackingLogic <--------------------------

template<class UnpackSectionStrategy, typename... OptionalParams>
void commonSectionUnpackingLogic(UnpackSectionStrategy unpackSectionStrategy, GlobalPalette& globalPalette, SectionPack& section, int32_t xOffset, int32_t zOffset, OptionalParams&&... optionalParams) {
    // generate localToGlobalPaletteIndex array
    uint32_t localPaletteSize = section.blockStates.palleteList.size();
    uint32_t localToGlobalPaletteIndex[localPaletteSize];
    for(uint32_t i = 0; i < localPaletteSize; i++) {
        localToGlobalPaletteIndex[i] = globalPalette[section.blockStates.palleteList[i].name];
    }

    // Handle unary section
    if (localPaletteSize == 1) {
        uint32_t index = 0;
        for (uint32_t j = 0; j < 16; j++) {
            for (uint32_t k = 0; k < 16; ++k) {
                for (uint32_t i = 0; i < 16; ++i) {
                    unpackSectionStrategy.insert(index, i + xOffset, j + section.yOffset, k + zOffset, localToGlobalPaletteIndex[0], optionalParams...);
                    ++index; 
                }
            }
        }
        return;
    }

    // calculate min number of bits to represent palette index
    uint32_t num_bits = helpers::bitLength(localPaletteSize);
    if (num_bits < 4) num_bits = 4;

    // make bitmask
    uint64_t bitmask = ((1ULL << num_bits) - 1);

    uint32_t indexes_per_element = 64 / num_bits;
    uint32_t last_state_elements = SECTION_SIZE % indexes_per_element;
    if (last_state_elements == 0) last_state_elements = indexes_per_element;

    // First loop
    for (int i = 0; i < section.blockStates.dataListLength - 1; ++i) {
        uint64_t word = section.blockStates.dataList[i];

        for (int j = 0; j < indexes_per_element; ++j) {
            uint32_t globalIndex = i * indexes_per_element + j;
            uint32_t localPaletteIndex = (uint32_t)(word & bitmask);
            
            uint32_t localX, localY, localZ;
            helpers::sectionDataIndexToLocalCoords(globalIndex, localX, localY, localZ);

            unpackSectionStrategy.insert(globalIndex, localX + xOffset, localY + section.yOffset, localZ + zOffset, localToGlobalPaletteIndex[localPaletteIndex], optionalParams...);
            
            word = word >> num_bits;
        }
    }

    // Final word
    int32_t finalWordIndex = section.blockStates.dataListLength - 1;
    int64_t finalWord = section.blockStates.dataList[finalWordIndex];
    for (int j = 0; j < last_state_elements; ++j) {
        uint32_t globalIndex = finalWordIndex * indexes_per_element + j;
        uint32_t localPaletteIndex = (uint32_t)(finalWord & bitmask);
        
        uint32_t localX, localY, localZ;
        helpers::sectionDataIndexToLocalCoords(globalIndex, localX, localY, localZ);

        unpackSectionStrategy.insert(globalIndex, localX + xOffset, localY + section.yOffset, localZ + zOffset, localToGlobalPaletteIndex[localPaletteIndex], optionalParams...);
        
        finalWord = finalWord >> num_bits;
    }
}

// --------------------------> BaseUnpackSectionStrategy <--------------------------

struct BaseUnpackSectionStrategy {
    inline void insert(uint32_t dataIndex, int32_t i, int32_t j, int32_t k, int32_t paletteIndex, auto& optionalParams) {}
};

// --------------------------> SectionToCoordsStrategy <--------------------------

struct SectionToCoordsStrategy : BaseUnpackSectionStrategy {
    inline void insert(uint32_t dataIndex, int32_t i, int32_t j, int32_t k, int32_t paletteIndex, int32_t* i_coords, int32_t* j_coords, int32_t* k_coords, int32_t* palette_indices) {
        i_coords[dataIndex] = i;
        j_coords[dataIndex] = j;
        k_coords[dataIndex] = k;
        palette_indices[dataIndex] = paletteIndex;
    }
};

// --------------------------> sectionToCoords <--------------------------

/// @brief parses a sectionList into a list of ijks and associated block
void sectionToCoords(GlobalPalette& globalPalette, SectionPack& section, int32_t xOffset, int32_t zOffset, int32_t* i_coords, int32_t* j_coords, int32_t* k_coords, int32_t* palette_indices) {
    SectionToCoordsStrategy strategy;
    commonSectionUnpackingLogic(strategy, globalPalette, section, xOffset, zOffset, i_coords, j_coords, k_coords, palette_indices);
}

// --------------------------> sectionListToCoords <--------------------------

void sectionListToCoords(GlobalPalette& globalPalette, SectionListPack& sectionList, int32_t* i_coords, int32_t* j_coords, int32_t* k_coords, int32_t* palette_indices) {
    SectionToCoordsStrategy strategy;
    uint32_t numSections = sectionList.size();

    // w << 12 = w*SECTION_SIZE
    for (uint32_t w = 0; w < numSections; ++w) {
        commonSectionUnpackingLogic(strategy, globalPalette, sectionList[w], sectionList.xOffset, sectionList.zOffset, i_coords + (w << 12), j_coords + (w << 12), k_coords + (w << 12), palette_indices + (w << 12));
    }
}

} // namespace NBTParser

#endif