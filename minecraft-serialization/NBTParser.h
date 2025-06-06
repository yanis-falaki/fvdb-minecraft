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
#include <memory>
#include <variant>

namespace NBTParser {

// --------------------------> constant values <--------------------------
static constexpr size_t SECTION_SIZE = 4096;
static constexpr size_t SECTION_LENGTH = 16;
static constexpr size_t MAX_CHUNKS_IN_REGION = 1024;

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

    // Check if a name exists in the map
    bool nameExists(const std::string& name) const {
        return nameToIndexMap.find(name) != nameToIndexMap.end();
    }

    uint32_t size() { return indexToStringVector.size(); }
};

// --------------------------> Tags enum <--------------------------

enum class Tag : uint8_t {
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

// --------------------------> Forward Declarations <--------------------------

class NBTCompound;
template<Tag TAG> class NBTList;

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

template<>
struct TagType<Tag::Byte_Array> {
    using Type = std::vector<int8_t>;
};

template<>
struct TagType<Tag::String> {
    using Type = std::string;
};

template<>
struct TagType<Tag::List> {
    using Type = std::variant<
        std::unique_ptr<NBTList<Tag::Byte>>,
        std::unique_ptr<NBTList<Tag::Short>>,
        std::unique_ptr<NBTList<Tag::Int>>,
        std::unique_ptr<NBTList<Tag::Long>>,
        std::unique_ptr<NBTList<Tag::Float>>,
        std::unique_ptr<NBTList<Tag::Double>>,
        std::unique_ptr<NBTList<Tag::Byte_Array>>,
        std::unique_ptr<NBTList<Tag::String>>,
        std::unique_ptr<NBTList<Tag::List>>,
        std::unique_ptr<NBTList<Tag::Compound>>,
        std::unique_ptr<NBTList<Tag::Int_Array>>,
        std::unique_ptr<NBTList<Tag::Long_Array>>
    >;
};


template<>
struct TagType<Tag::Compound> {
    using Type = std::unique_ptr<NBTCompound>;
};

template<>
struct TagType<Tag::Int_Array> {
    using Type = std::vector<int32_t>;
};

template<>
struct TagType<Tag::Long_Array> {
    using Type = std::vector<int64_t>;
};


// --------------------------> toStr <--------------------------

// Function to convert Tag to string
template<Tag T>
constexpr const char* tagToStr() {
    if constexpr (T == Tag::End)         return "End";
    else if constexpr (T == Tag::Byte)   return "Byte";
    else if constexpr (T == Tag::Short)  return "Short";
    else if constexpr (T == Tag::Int)    return "Int";
    else if constexpr (T == Tag::Long)   return "Long";
    else if constexpr (T == Tag::Float)  return "Float";
    else if constexpr (T == Tag::Double) return "Double";
    else if constexpr (T == Tag::Byte_Array) return "Byte_Array";
    else if constexpr (T == Tag::String)     return "String";
    else if constexpr (T == Tag::List)       return "List";
    else if constexpr (T == Tag::Compound)   return "Compound";
    else if constexpr (T == Tag::Int_Array)  return "Int_Array";
    else if constexpr (T == Tag::Long_Array) return "Long_Array";
    else return "Unknown";
}


template<typename T>
constexpr const char* tagTypeToStr() {
    if constexpr (std::is_same_v<T, TagType<Tag::Byte>::Type>)       return "Byte";
    else if constexpr (std::is_same_v<T, TagType<Tag::Short>::Type>) return "Short";
    else if constexpr (std::is_same_v<T, TagType<Tag::Int>::Type>)   return "Int";
    else if constexpr (std::is_same_v<T, TagType<Tag::Long>::Type>)  return "Long";
    else if constexpr (std::is_same_v<T, TagType<Tag::Float>::Type>) return "Float";
    else if constexpr (std::is_same_v<T, TagType<Tag::Double>::Type>)return "Double";
    else if constexpr (std::is_same_v<T, TagType<Tag::String>::Type>)return "String";
    else if constexpr (std::is_same_v<T, TagType<Tag::Compound>::Type>) return "Compound";
    else if constexpr (std::is_same_v<T, TagType<Tag::Byte_Array>::Type>) return "Byte_Array";
    else if constexpr (std::is_same_v<T, TagType<Tag::Int_Array>::Type>) return "Int_Array";
    else if constexpr (std::is_same_v<T, TagType<Tag::Long_Array>::Type>) return "Long_Array";
    else if constexpr (std::is_same_v<T, TagType<Tag::List>::Type>) return "List";
    else return "Unknown";
}


// --------------------------> payloadLengthMap <--------------------------

// If the payload could have variable length or requires special preprocessing then length is set to 0
constexpr size_t PAYLOAD_LENGTH_MAP[] = {
    1,   // End,
    sizeof(TagType<Tag::Byte>::Type),   // Byte
    sizeof(TagType<Tag::Short>::Type),   // Short
    sizeof(TagType<Tag::Int>::Type),   // Int
    sizeof(TagType<Tag::Long>::Type),   // Long
    sizeof(TagType<Tag::Float>::Type),   // Float
    sizeof(TagType<Tag::Double>::Type),   // Double
    sizeof(TagType<Tag::Byte_Array>::Type),  // Byte_Array
    sizeof(TagType<Tag::String>::Type),  // String
    sizeof(TagType<Tag::List>::Type),  // List
    sizeof(TagType<Tag::Compound>::Type),  // Compound
    sizeof(TagType<Tag::Int_Array>::Type),  // Int_Array
    sizeof(TagType<Tag::Long_Array>::Type)   // Long_Array
};

// --------------------------> getPayloadLength <--------------------------

// Function to get payload length for a given Tag
constexpr size_t getPayloadLength(Tag tag) {
    return PAYLOAD_LENGTH_MAP[static_cast<int>(tag)];
}
constexpr size_t getPayloadLength(uint8_t tag) {
    return PAYLOAD_LENGTH_MAP[tag];
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

// ------------------> Parser Function Forward Declarations <------------------

void parseNBTCompound(uint8_t*& iterator, NBTCompound& parent);
TagType<Tag::List>::Type parseNBTList(uint8_t*& iterator);

// --------------------------> NBTValue <--------------------------

using NBTValue = std::variant<
    TagType<Tag::Byte>::Type,       // Byte         int8_t
    TagType<Tag::Short>::Type,      // Short        int16_t
    TagType<Tag::Int>::Type,        // Int          int32_t
    TagType<Tag::Long>::Type,       // Long         int64_t
    TagType<Tag::Float>::Type,      // Float        float
    TagType<Tag::Double>::Type,     // Double       double
    TagType<Tag::String>::Type,     // String       std::string
    TagType<Tag::List>::Type,
    TagType<Tag::Compound>::Type,   // Compound     std::unique_ptr<NBTCompound>
    TagType<Tag::Byte_Array>::Type, // Byte Array   std::vector<int8_t>
    TagType<Tag::Int_Array>::Type,  // Int Array    std::vector<int32_t>
    TagType<Tag::Long_Array>::Type  // Long Array   std::vector<int64_t>
>;

// --------------------------> NBTCompound <--------------------------

class NBTCompound {
private:
    std::unordered_map<std::string, NBTValue> mMap;

public:
    NBTCompound() = default;
    NBTCompound(uint8_t* iterator) {
        parseNBTCompound(iterator, *this);
    }

    const NBTValue& getValue(const std::string& key) const {
        return mMap.at(key);
    }

    void printAll(uint32_t depth=0) const {
        for (const auto& [key, value] : mMap) {
            std::visit([&key, depth](const auto& val) {
                using T = std::decay_t<decltype(val)>;

                std::cout << std::string(depth * 2, ' ');
                std::cout << helpers::colorKey << key << helpers::colorReset << '(' 
                          << helpers::colorTag << tagTypeToStr<T>() << helpers::colorReset << ')' << ": ";

                if constexpr (std::is_same_v<T, TagType<Tag::Compound>::Type>) {
                    std::cout << std::endl;
                    val->printAll(depth+1);
                    return;
                }
                else if constexpr (std::is_same_v<T, TagType<Tag::List>::Type>) {
                    std::cout << std::endl;
                    std::visit([depth](const auto& listPtr) {
                        if (listPtr) {
                            listPtr->printAll(depth + 1);
                        } else {
                            std::cout << "[null list pointer]" << std::endl;
                        }
                        return;
                    }, val);
                    return;
                }
                else if constexpr (std::is_same_v<T, TagType<Tag::Byte_Array>::Type> ||
                                   std::is_same_v<T, TagType<Tag::Int_Array>::Type> ||
                                   std::is_same_v<T, TagType<Tag::Long_Array>::Type>) {
                    std::cout << "Length: " << val.size();
                }
                else if constexpr (std::is_same_v<T, TagType<Tag::Byte>::Type> ||
                              std::is_same_v<T, TagType<Tag::Short>::Type> ||
                              std::is_same_v<T, TagType<Tag::Int>::Type> ||
                              std::is_same_v<T, TagType<Tag::Long>::Type> ||
                              std::is_same_v<T, TagType<Tag::Float>::Type> ||
                              std::is_same_v<T, TagType<Tag::Double>::Type> ||
                              std::is_same_v<T, TagType<Tag::String>::Type>) {
                    std::cout << val;
                }
                std::cout << std::endl;
            }, value);
        }
    }

    void makeByteChild(uint8_t value, std::string name) {
        mMap.emplace(name, value);
    }

    template<Tag NumT>
    void makeNumericChild(auto value, std::string name) {
        static_assert((NumT == Tag::Byte) || (NumT == Tag::Short) || (NumT == Tag::Int) ||
                      (NumT == Tag::Long) || (NumT == Tag::Float) || (NumT == Tag::Double));
        mMap.emplace(name, value);
    }

    void makeStringChild(std::string value, std::string name) {
        mMap.emplace(name, value);
    }

    void makeByteArrayChild(uint8_t* startPtr, uint32_t length, std::string name) {
        mMap.emplace(name, std::vector<int8_t>(startPtr, startPtr + length));
    }

    void makeIntArrayChild(uint8_t* startPtr, uint32_t length, std::string name) {
        std::vector<int32_t> vector(length);
        for (uint32_t i = 0; i < length; ++i) {
            vector[i] = readNum<Tag::Int>(startPtr + i * 4);
        }
        mMap.emplace(name, std::move(vector));
    }

    void makeLongArrayChild(uint8_t* startPtr, uint32_t length, std::string name) {
        std::vector<int64_t> vector(length);
        for (uint32_t i = 0; i < length; ++i) {
            vector[i] = readNum<Tag::Long>(startPtr + i * 8);
        }
        mMap.emplace(name, std::move(vector));
    }

    NBTCompound* makeCompoundChild(std::string name) {
        auto child = std::make_unique<NBTCompound>();
        auto ptr = child.get();
        mMap.emplace(name, std::move(child));
        return ptr;
    }

    void makeListChild(TagType<Tag::List>::Type&& nbtList, std::string name) {
        mMap.emplace(name, std::move(nbtList));
    }
};

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

// --------------------------> parseNBTCompound (main loop) <--------------------------

void parseNBTCompound(uint8_t*& iterator, NBTCompound& parent) {
    while (true) {
        auto tagAndName = parseTagAndName(iterator);
        if (tagAndName.isEnd) return;

        switch (tagAndName.tag) {
            case Tag::Byte: {
                auto value = readNum<Tag::Byte>(iterator);
                parent.makeByteChild(value, tagAndName.name);
                iterator += getPayloadLength(Tag::Byte);
                break;
            }
            case Tag::Short: {
                auto value = readNum<Tag::Short>(iterator);
                parent.makeNumericChild<Tag::Short>(value, tagAndName.name);
                iterator += getPayloadLength(Tag::Short);
                break;
            }
            case Tag::Int: { 
                auto value = readNum<Tag::Int>(iterator);
                parent.makeNumericChild<Tag::Int>(value, tagAndName.name);
                iterator += getPayloadLength(Tag::Int);
                break;
            }
            case Tag::Long: { 
                auto value = readNum<Tag::Long>(iterator);
                parent.makeNumericChild<Tag::Long>(value, tagAndName.name);
                iterator += getPayloadLength(Tag::Long);
                break;
            }
            case Tag::Float: { 
                auto value = readNum<Tag::Float>(iterator);
                parent.makeNumericChild<Tag::Float>(value, tagAndName.name);
                iterator += getPayloadLength(Tag::Float);
                break;
            }
            case Tag::Double: { 
                auto value = readNum<Tag::Double>(iterator);
                parent.makeNumericChild<Tag::Double>(value, tagAndName.name);
                iterator += getPayloadLength(Tag::Double);
                break;
            }

            case Tag::Byte_Array: {
                uint32_t length = (*iterator << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | *(iterator + 3);
                parent.makeByteArrayChild(iterator + 4, length, tagAndName.name);
                iterator += 4 + length;
                break;
            }
            
            case Tag::String: {
                uint16_t stringLength = (*iterator << 8) | (*(iterator + 1));
                char stringArray[stringLength + 1];
                helpers::strcpy(iterator+2, stringArray, stringLength);
                parent.makeStringChild(stringArray, tagAndName.name);
                iterator += 2 + stringLength;
                break;
            }
            
            case Tag::List: {
                TagType<Tag::List>::Type listPtr = parseNBTList(iterator);
                parent.makeListChild(std::move(listPtr), tagAndName.name);
                break;
            }
            
            case Tag::Compound: {
                auto childPtr = parent.makeCompoundChild(tagAndName.name);
                parseNBTCompound(iterator, *childPtr);
                break;
            }

            case Tag::Int_Array: {
                int32_t length = readNum<Tag::Int>(iterator);
                parent.makeIntArrayChild(iterator + 4, length, tagAndName.name);
                iterator += 4 + 4*length;
                break;
            }

            case Tag::Long_Array: {
                int32_t length = (*(iterator) << 24) | (*(iterator + 1) << 16) | (*(iterator + 2) << 8) | (*(iterator + 3));
                parent.makeLongArrayChild(iterator + 4, length, tagAndName.name);
                iterator += 4 + 8*length;
                break;
            }
        }
    }
}

// --------------------------> NBTList <--------------------------

template <Tag TAG>
class NBTList {
private:
    using ValueType = typename TagType<TAG>::Type;
    std::vector<ValueType> mValues;

    static std::vector<ValueType> readNumericValues(uint8_t*& startPtr, int32_t length) {
        constexpr size_t payloadLength = getPayloadLength(TAG);
        std::vector<ValueType> values(length);
        for (size_t i = 0; i < length; i++) {
            values[i] = readNum<TAG>(startPtr);
            startPtr += payloadLength;
        }
        return values;
    }

public:
    ~NBTList() = default;

    // copying has implications I don't want to deal with for now.
    NBTList(const NBTList& other) = default;
    NBTList& operator=(const NBTList& other) = default;

    NBTList(NBTList&& other) = default;
    NBTList& operator=(NBTList&& other) = default;

    const std::vector<ValueType>& getValues() const {
        return mValues;
    }

    const void printAll(uint32_t depth = 0) {
        std::cout << std::string(depth * 2, ' ');
        std::cout << "Tag: " << helpers::colorTag << tagToStr<TAG>()
                << helpers::colorReset << " Length: " << mValues.size() << std::endl;

        if constexpr (TAG == Tag::Compound) {
            for (size_t i = 0; i < mValues.size(); ++i) {
                auto& compoundPtr = mValues[i];
                std::cout << std::string(depth * 2, ' ') << "Compound #" << i << std::endl;
                if (compoundPtr) {
                    compoundPtr->printAll(depth + 1);
                } else {
                    std::cout << std::string((depth + 1) * 2, ' ') << "[null compound]" << std::endl;
                }
            }
        }

        return;
    }

    // Byte
    NBTList(uint8_t*& startPtr, int32_t length) requires (TAG == Tag::Byte) {
        std::vector<ValueType> values(startPtr, startPtr + length);
        mValues = std::move(values);
        startPtr += length;
    }

    // Numeric
    NBTList(uint8_t*& startPtr, int32_t length) requires (
        TAG == Tag::Short || TAG == Tag::Int || TAG == Tag::Long ||
        TAG == Tag::Float || TAG == Tag::Double
    ) {
        mValues = readNumericValues(startPtr, length);
    };

    // Byte_Array
    NBTList(uint8_t*& startPtr, int32_t length) requires (TAG == Tag::Byte_Array) {
        mValues.resize(length);
        for (size_t i = 0; i < length; ++i) {
            int32_t currentArrayLength = readNum<Tag::Int>(startPtr);
            startPtr += 4;
            mValues[i] = std::vector<typename TagType<Tag::Byte>::Type>(startPtr, startPtr + currentArrayLength);
            startPtr += currentArrayLength;
        }
    }

    // String
    NBTList(uint8_t*& startPtr, int32_t length) requires (TAG == Tag::String) {
        mValues.resize(length);
        for (size_t i = 0; i < length; ++i) {
            uint16_t stringLength = (*startPtr << 8) | (*(startPtr + 1)); // unsigned short not implemented in readNum
            startPtr += 2;
            mValues[i] = std::string(reinterpret_cast<const char*>(startPtr), stringLength);
            startPtr += stringLength;
        }
    }

    // TODO implement nested lists
    NBTList(uint8_t* startPtr, int32_t length) requires (TAG == Tag::List) {
        return;
    }

    // Compound
    NBTList(uint8_t*& startPtr, int32_t length) requires (TAG == Tag::Compound) {
        mValues.resize(length);
        for (size_t i = 0; i < length; ++i) {
            auto compound = std::make_unique<NBTCompound>();
            parseNBTCompound(startPtr, *compound);
            mValues[i] = std::move(compound);
        }
    }

    // Int_Array
    NBTList(uint8_t*& startPtr, int32_t length) requires (TAG == Tag::Int_Array) {
        mValues.resize(length);
        for (size_t i = 0; i < length; ++i) {
            int32_t currentArrayLength = readNum<Tag::Int>(startPtr);
            startPtr += 4;
            mValues[i].resize(currentArrayLength);
            for (size_t j = 0; j < currentArrayLength; ++j) {
                mValues[i][j] = readNum<Tag::Int>(startPtr);
                startPtr += 4;
            }
        }
    }

    // Long_Array
    NBTList(uint8_t*& startPtr, int32_t length) requires (TAG == Tag::Long_Array) {
        mValues.resize(length);
        for (size_t i = 0; i < length; ++i) {
            int32_t currentArrayLength = readNum<Tag::Int>(startPtr);
            startPtr += 4;
            mValues[i].resize(currentArrayLength);
            for (size_t j = 0; j < currentArrayLength; ++j) {
                mValues[i][j] = readNum<Tag::Long>(startPtr);
                startPtr += 8;
            }
        }
    }
};

// --------------------------> parseNBTList <--------------------------

TagType<Tag::List>::Type parseNBTList(uint8_t*& iterator) {
    Tag tag = static_cast<Tag>(*iterator);
    int32_t listLength = readNum<Tag::Int>(iterator+1);

    TagType<Tag::List>::Type listPtr;

    iterator += 5; // skip past tag and listLength

    switch(tag) {
        case Tag::Byte:
            listPtr = std::make_unique<NBTList<Tag::Byte>>(iterator, listLength);
            break;
        case Tag::Short: 
            listPtr = std::make_unique<NBTList<Tag::Short>>(iterator, listLength);
            break;
        case Tag::Int: 
            listPtr = std::make_unique<NBTList<Tag::Int>>(iterator, listLength);
            break;
        case Tag::Long: 
            listPtr = std::make_unique<NBTList<Tag::Long>>(iterator, listLength);
            break;
        case Tag::Float: 
            listPtr = std::make_unique<NBTList<Tag::Float>>(iterator, listLength);
            break;
        case Tag::Double: 
            listPtr = std::make_unique<NBTList<Tag::Double>>(iterator, listLength);
            break;
        case Tag::Byte_Array:
            listPtr = std::make_unique<NBTList<Tag::Byte_Array>>(iterator, listLength);
            break;
        case Tag::String:
            listPtr = std::make_unique<NBTList<Tag::String>>(iterator, listLength);
            break;
        case Tag::List:
            listPtr = std::make_unique<NBTList<Tag::List>>(iterator, listLength);
            break;
        case Tag::Compound:
            listPtr = std::make_unique<NBTList<Tag::Compound>>(iterator, listLength);
            break;
        case Tag::Int_Array:
            listPtr = std::make_unique<NBTList<Tag::Int_Array>>(iterator, listLength);
            break;
        case Tag::Long_Array:
            listPtr = std::make_unique<NBTList<Tag::Long_Array>>(iterator, listLength);
            break;
    }
    return listPtr;
}

} // namespace NBTParser

#endif