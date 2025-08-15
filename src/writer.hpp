#ifndef WRITER_HPP
#define WRITER_HPP

#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "reader.hpp"

#include <string>
#include <fstream>

class Writer : public Reader {
private:
    XRefTable xref;

    bool writeXref(std::ofstream& outfile);

public:

    // Returns true on success, false on failure.
    bool writeNewFile(const std::string& filename);


};

#endif