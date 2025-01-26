# ifndef HELPERS_INCLUDED
# define HELPERS_INCLUDED

#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <constants.h>
#include <cstring>

using namespace constants;

namespace helpers 
{

// --------------------------> Object Dump (used for debugging) <--------------------------

void hexDumpArrayToFile(const unsigned char *data, size_t size, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    for (size_t i = 0; i < size; i++) {
        fprintf(file, "%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            fprintf(file, "\n");
        }
    }
    if (size % 16 != 0) {
        fprintf(file, "\n");
    }

    fclose(file);
}

void hexDumpVectorToFile(const std::vector<unsigned char>& vec, const char* filename) {
    int size = vec.size();

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    for (size_t i = 0; i < size; i++) {
        fprintf(file, "%02X ", vec[i]);
        if ((i + 1) % 16 == 0) {
            fprintf(file, "\n");
        }
    }
    if (size % 16 != 0) {
        fprintf(file, "\n");
    }

    fclose(file);
}


void dumpArrayToFile(const unsigned char* array, size_t size, const char* filename) {
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(array), size);

    if (file.fail()) {
        std::cerr << "Error writing to file: " << filename << std::endl;
    }

    file.close();
}

void dumpVectorToFile(const std::vector<unsigned char>& vec, const char* filename) {
    std::ofstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(vec.data()), vec.size());

    if (file.fail()) {
        std::cerr << "Error writing to file: " << filename << std::endl;
    }

    file.close();
}

// --------------------------> Uncompression <--------------------------

std::vector<uint8_t> uncompress_chunk(const uint8_t* compressed_data, uLong compressed_size) {
    uLong dest_len = compressed_size * 10;
    std::vector<uint8_t> destination(dest_len);
    
    while (true) {
        int result = uncompress(destination.data(), &dest_len, compressed_data, compressed_size);
        
        if (result == Z_OK) {
            destination.resize(dest_len);
            return destination;
        }
        
        if (result == Z_BUF_ERROR) {
            dest_len *= 2;
            destination.resize(dest_len);
            continue;
        }
        
        throw std::runtime_error("Decompression failed");
    }
}

uint8_t* uncompress_chunk(const uint8_t* compressed_data, uLong compressed_size, uint32_t& uncompressed_size) {
    uLong dest_len = compressed_size * 10;
    uint8_t* destination = new uint8_t[dest_len];
    
    while (true) {
        int result = uncompress(destination, &dest_len, compressed_data, compressed_size);
        
        if (result == Z_OK) {
            uncompressed_size = dest_len;
            return destination;
        }
        
        if (result == Z_BUF_ERROR) {
            delete[] destination;
            dest_len *= 2;
            destination = new uint8_t[dest_len];
            continue;
        }
        
        delete[] destination;
        throw std::runtime_error("Decompression failed");
    }
}


// --------------------------> Tags <--------------------------

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

// Function to get payload length for a given Tag
inline uint8_t getPayloadLength(Tag tag) {
    return payloadLengthMap[static_cast<int>(tag)];
}
inline uint8_t getPayloadLength(uint8_t tag) {
    return payloadLengthMap[tag];
}

// --------------------------> strcpy <--------------------------

template <typename IteratorType>
inline void strcpy(IteratorType iterator, char* dest, uint8_t n) {
    dest[n] = '\0';
    for (int i = 0; i < n; ++i) {
        dest[i] = *(iterator + i);
    }
}

// --------------------------> Parse Tag <--------------------------

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

} // namespace helpers

#endif // ifndef HELPERS