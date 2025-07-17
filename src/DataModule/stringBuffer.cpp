#include "stringBuffer.hpp"

#include <string>
#include <vector>

uint64_t StringBuffer::addString(std::string str) {
    uint64_t currentOffset = offset;

    // Append the bytes of the string
    buffer.insert(buffer.end(), str.begin(), str.end());

    // Update offset
    offset += str.size();

    return currentOffset;
}