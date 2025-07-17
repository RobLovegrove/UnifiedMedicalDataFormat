#ifndef STRINGBUFFER_HPP
#define STRINGBUFFER_HPP

#include <vector>
#include <string>
#include <fstream>

class StringBuffer {
private:
    std::vector<uint8_t> buffer;
    uint64_t offset = 0;

public:
    uint64_t addString(std::string string);
    size_t getSize() const { return buffer.size(); }

    const std::vector<uint8_t>& getBuffer() const;

    void writeToFile(std::ostream& out);
    void readFromFile(std::istream& in, size_t size);

};




#endif
