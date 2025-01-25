#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED

#include <cstdint>

namespace constants
{

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


// If the payload could have variable length or requires special preprocessing then length is set to 0
const uint8_t payloadLengthMap[] = {
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
  
}

#endif