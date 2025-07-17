#include "stringBuffer.hpp"

#include <string>
#include <vector>

using namespace std;

uint64_t StringBuffer::addString(std::string str) {
    uint64_t currentOffset = offset;

    // Append the bytes of the string
    buffer.insert(buffer.end(), str.begin(), str.end());

    // Update offset
    offset += str.size();

    return currentOffset;
}

void StringBuffer::writeToFile(ostream& out) {
    if (!out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size())) {
        throw runtime_error("Failed to write string buffer to output stream");
    }
}

void StringBuffer::readFromFile(std::istream& in, size_t size) {
    buffer.resize(size);  // Resize internal buffer to hold the data

    // Read 'size' bytes into buffer.data()
    in.read(reinterpret_cast<char*>(buffer.data()), size);

    // Check if read succeeded
    if (!in) {
        throw std::runtime_error("Failed to read string buffer from stream");
    }

    // Update offset to match buffer size
    offset = static_cast<uint64_t>(buffer.size());
}

const std::vector<uint8_t>& StringBuffer::getBuffer() const {
    return buffer;
}