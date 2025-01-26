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

namespace NBTParser {


bool findSectionsList(uint8_t*& iterator);
void printNBTStructure(uint8_t*& iterator);
inline bool skipNBTStructure(uint8_t*& iterator);

inline void printList(uint8_t*& iterator);
inline void skipList(uint8_t*& iterator);

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

// --------------------------> parseNBTStructure (main loop) <--------------------------

template<typename Strategy, typename... ExtraArgs>
bool parseNBTStructure(uint8_t*& iterator, Strategy& strategy, ExtraArgs&&... extraArgs) {
    while (true) {
        auto tagAndName = parseTagAndName(iterator);
        strategy.preamble(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...); // optional printing
        if (tagAndName.isEnd) return false;

        switch (tagAndName.tag) {
            case Tag::Byte: 
                strategy.template handleByteTag(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...);
                iterator += getPayloadLength(Tag::Byte);
                break;
            case Tag::Short: 
                strategy.template handleNumericTag<Tag::Short>(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...);
                iterator += getPayloadLength(Tag::Short);
                break;
            case Tag::Int: 
                strategy.template handleNumericTag<Tag::Int>(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...);
                iterator += getPayloadLength(Tag::Int);
                break;
            case Tag::Long: 
                strategy.template handleNumericTag<Tag::Long>(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...);
                iterator += getPayloadLength(Tag::Long);
                break;
            case Tag::Float: 
                strategy.template handleNumericTag<Tag::Float>(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...);
                iterator += getPayloadLength(Tag::Float);
                break;
            case Tag::Double: 
                strategy.template handleNumericTag<Tag::Double>(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...);
                iterator += getPayloadLength(Tag::Double);
                break;

            case Tag::Byte_Array: {
                uint32_t length = (*iterator << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | *(iterator + 3);
                strategy.handleByteArray(iterator, tagAndName, length, std::forward<ExtraArgs>(extraArgs)...);
                iterator += 4 + length;
                break;
            }

            case Tag::String: {
                uint16_t stringLength = (*iterator << 8) | (*(iterator + 1));
                strategy.handleString(iterator, tagAndName, stringLength, std::forward<ExtraArgs>(extraArgs)...);
                iterator += 2 + stringLength;
                break;
            }

            case Tag::List: {
                if (strategy.handleList(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...))
                    return true;
                break;
            }

            case Tag::Compound: {
                if (strategy.handleCompound(iterator, tagAndName, std::forward<ExtraArgs>(extraArgs)...))
                    return true;
                break;
            }

            case Tag::Int_Array: {
                int size = (*(iterator) << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | (*(iterator + 3));
                iterator += 4 + size*4;
                break;
            }

            case Tag::Long_Array: {
                int size = (*(iterator) << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | (*(iterator + 3));
                iterator += 4 + size*8;
                break;
            }
        }
    }
}

// --------------------------> ParsingStrategies <--------------------------

struct FindSectionsListStrategy {
    inline void preamble(uint8_t*& iterator, const auto& tagAndName) {}
    inline void handleByteTag(uint8_t*& iterator, const auto& tagAndName) {}
    template<Tag NumTag>
    inline void handleNumericTag(uint8_t*& iterator, const auto& tagAndName) {}
    inline void handleByteArray(uint8_t*& iterator, const auto& tagAndName, uint32_t length) {}
    inline void handleString(uint8_t*& iterator, const auto& tagAndName, uint16_t stringLength) {}

    inline bool handleList(uint8_t*& iterator, const auto& tagAndName) {
        if (tagAndName.name == "sections") return true;
        printList(iterator);
        return false;
    }
    inline bool handleCompound(uint8_t*& iterator, const auto& tagAndName) {
        return skipNBTStructure(iterator);
    }
};

struct SkipNBTStructureStrategy {
    inline void preamble(uint8_t*& iterator, const auto& tagAndName) {}
    inline void handleByteTag(uint8_t*& iterator, const auto& tagAndName) {}
    template<Tag NumTag>
    inline void handleNumericTag(uint8_t*& iterator, const auto& tagAndName) {}
    inline void handleByteArray(uint8_t*& iterator, const auto& tagAndName, uint32_t length) {}
    inline void handleString(uint8_t*& iterator, const auto& tagAndName, uint16_t stringLength) {}
    inline bool handleList(uint8_t*& iterator, const auto& tagAndName) {
        skipList(iterator);
        return false;
    }
    inline bool handleCompound(uint8_t*& iterator, const auto& tagAndName) {
        return skipNBTStructure(iterator);
    }
};

struct PrintNBTStructureStrategy {
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
        for (int i = 0; i < length; ++i) {
            std::cout << (short)(*(iterator + 4 + i));
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


struct SectionCompoundStrategy {
    inline void preamble(uint8_t*& iterator, const auto& tagAndName, int32_t& y) {}
    inline void handleByteTag(uint8_t*& iterator, const auto& tagAndName, int32_t& y) {
        y = readNum<Tag::Byte>(iterator);
    }
    template<Tag NumTag>
    inline void handleNumericTag(uint8_t*& iterator, const auto& tagAndName, int32_t& y) {
        std::cerr << "Encountered a non Byte numeric tag" << std::endl;
    }
    inline void handleByteArray(uint8_t*& iterator, const auto& tagAndName, uint32_t length, int32_t& y) {}
    inline void handleString(uint8_t*& iterator, const auto& tagAndName, uint16_t stringLength, int32_t& y) {}
    inline bool handleList(uint8_t*& iterator, const auto& tagAndName, int32_t& y) {
        skipList(iterator);
        return false;
    }
    inline bool handleCompound(uint8_t*& iterator, const auto& tagAndName, int32_t& y) {
        return skipNBTStructure(iterator);
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
    SkipNBTStructureStrategy strategy;
    return parseNBTStructure(iterator, strategy);
}

inline void printNBTStructure(uint8_t*& iterator) {
    PrintNBTStructureStrategy strategy;
    parseNBTStructure(iterator, strategy);
}

inline void sectionCompoundStrategy(uint8_t*& iterator, int32_t& y) {
    SectionCompoundStrategy strategy;
    parseNBTStructure(iterator, strategy, y);
}

// --------------------------> exploreList <--------------------------

template<typename ListStrategy>
void exploreList(uint8_t*& iterator, ListStrategy listStrategy) {
    uint8_t payloadTagLength =  getPayloadLength(*iterator);
    Tag list_tag = static_cast<Tag>(*iterator);
    int32_t listLength = readNum<Tag::Int>(iterator+1);

    iterator += 5 + (payloadTagLength * listLength);

    listStrategy.preamble(list_tag, listLength);

    if (list_tag == Tag::End) return;

    // If atypical list_tag, then the iterator will have only jumped 5 units as payloadTagLength will be zero
    if (list_tag == Tag::Compound) {
        listStrategy.handleCompound(iterator, listLength);
    }
    else if (list_tag == Tag::String) {
        listStrategy.handleString(iterator, listLength);
    }
    else if (list_tag == Tag::List) {
        listStrategy.handleList(iterator, listLength);
    }
}

// --------------------------> List Strategies <--------------------------

struct PrintListStrategy {
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

struct SkipListStrategy {
    inline void preamble(Tag list_tag, int32_t listLength) {}

    inline void handleCompound(uint8_t*& iterator, uint32_t listLength) {
        for (int i = 0; i < listLength; ++i) skipNBTStructure(iterator);
    }

    inline void handleString(uint8_t*& iterator, uint32_t listLength) {
        for (int i = 0; i < listLength; ++i) {
            int32_t stringLength = (*iterator << 8) | (*(iterator + 1));
            iterator += 2 + stringLength;
        }
    }

    inline void handleList(uint8_t*& iterator, uint32_t listLength) {
        for (int i = 0; i < listLength; ++i) skipList(iterator);
    }
};

// --------------------------> List Strategy Wrappers <--------------------------

inline void printList(uint8_t*& iterator) {
    PrintListStrategy listStrategy;
    exploreList(iterator, listStrategy);
}

inline void skipList(uint8_t*& iterator) {
    SkipListStrategy listStrategy;
    exploreList(iterator, listStrategy);
}


} // namespace NBTParser

#endif