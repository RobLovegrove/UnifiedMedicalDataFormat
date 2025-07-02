#ifndef READER_HPP
#define READER_HPP

#include <string>

#include "header.hpp"
#include "xref.hpp"

class Reader {
private: 
    Header header = Header();
    XRefTable xrefTable;

public:
    // Returns true on success (patientData filled), false on failure.
    bool readFile(const std::string& filename);

};

#endif