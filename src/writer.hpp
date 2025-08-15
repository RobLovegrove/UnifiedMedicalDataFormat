#ifndef WRITER_HPP
#define WRITER_HPP

#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "reader.hpp"

#include <string>
#include <fstream>

class Writer {
private:
    XRefTable xref;
    std::fstream* currentStream = nullptr;

    bool writeXref(std::ostream& outfile);

public:
    // Returns true on success, false on failure.
    bool writeNewFile(const std::string& filename);

    // Stream management
    void setStream(std::fstream& stream);
    
    // Python API
    bool addModule(std::fstream& fileStream, const ModuleData& module);
    bool updateModule(std::fstream& fileStream, const std::string& moduleId, const ModuleData& module);
    bool deleteModule(const std::string& moduleId);


};

#endif