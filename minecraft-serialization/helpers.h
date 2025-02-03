# ifndef HELPERS_INCLUDED
# define HELPERS_INCLUDED

#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>
#include <cstdint>
#include <cstring>

namespace NBTParser::helpers 
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
    uLong dest_len = compressed_size * 2;
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

// --------------------------> strcpy <--------------------------

template <typename IteratorType>
inline void strcpy(IteratorType iterator, char* dest, uint8_t n) {
    dest[n] = '\0';
    for (int i = 0; i < n; ++i) {
        dest[i] = *(iterator + i);
    }
}

// --------------------------> bit_length <--------------------------

uint32_t bitLength(uint32_t n) {
    return 32 -__builtin_clz(n);
}

// --------------------------> sectionDataIndexToLocalCoords <--------------------------

inline void sectionDataIndexToLocalCoords(uint32_t sectionDataIndex, uint32_t& localX, uint32_t& localY, uint32_t& localZ) {
    localY = sectionDataIndex / 256;
    uint32_t remainder = sectionDataIndex % 256;
    localZ = remainder / 16;
    localX = remainder % 16;
}

// --------------------------> localCoordsToSectionDataIndex <--------------------------

inline uint32_t localCoordsToSectionDataIndex(uint32_t localX, uint32_t localY, uint32_t localZ) {
    return (localY << 8) + (localZ << 4) + localX;
}

// --------------------------> globalCoordsToSectionDataIndex <--------------------------

inline uint32_t globalCoordsToSectionDataIndex(int32_t x, int32_t y, int32_t z) {
    return ((y & 15) << 8) + ((z & 15) << 4) + (x & 15);
}

// --------------------------> regionChunkIndexToGlobalChunkCoords <--------------------------

inline void regionChunkIndexToGlobalChunkCoords(uint32_t index, int32_t regionX, int32_t regionZ, int32_t& destChunkX, int32_t& destChunkZ) {
    destChunkX = (index & 31) + (regionX << 5);
    destChunkZ = (index >> 5) + (regionZ << 5);
    return;
}

// --------------------------> regionChunkIndexToLocalChunkCoords <--------------------------

inline void regionChunkIndexToLocalChunkCoords(uint32_t index, int32_t& destChunkX, int32_t& destChunkZ) {
    destChunkX = (index & 31);
    destChunkZ = (index >> 5);
    return;
}

// --------------------------> parseRegionCoordinatesFromString <--------------------------

inline void parseRegionCoordinatesFromString(const std::string& filename, int32_t& regionX, int32_t& regionZ) {
    // Navigate past the 'r.' prefix
    size_t startPos = 2;
    
    // Delineate coordinate boundaries via dot positions
    size_t firstDot = filename.find('.', startPos);
    size_t secondDot = filename.find('.', firstDot + 1);
    
    // Extract the coordinate substrings
    std::string xStr = filename.substr(startPos, firstDot - startPos);
    std::string zStr = filename.substr(firstDot + 1, secondDot - firstDot - 1);
    
    // Populate the reference parameters
    regionX = std::stoi(xStr);
    regionZ = std::stoi(zStr);
}

} // namespace helpers

#endif // ifndef HELPERS