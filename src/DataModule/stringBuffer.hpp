#ifndef STRINGBUFFER_HPP
#define STRINGBUFFER_HPP

#include <vector>
#include <string>


class StringBuffer {
private:
    std::vector<uint8_t> buffer;
    uint64_t offset = 0;

public:
    uint64_t addString(std::string string);


};




#endif
