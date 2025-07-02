#ifndef WRITER_HPP
#define WRITER_HPP

#include "header.hpp"
#include "xref.hpp"

#include <string>
#include <fstream>

enum class FileAccessMode {
    FailIfExists,  // Default: safe guard
    AllowUpdate,   // File may exist; update it
    Overwrite      // Rewrite entire file
};

class Writer {
private:
    FileAccessMode accessMode = FileAccessMode::FailIfExists;
    Header header = Header();
    XRefTable xref;

    bool writeHeader(std::ofstream& outfile);
    bool writeXref(std::ofstream& outfile);

public:

    // Returns true on success, false on failure.
    bool writeNewFile(const std::string& filename, const std::string& patientData);
    void setFileAccessMode(FileAccessMode);
    FileAccessMode getFileAccessMode() const { return accessMode; }

};

#endif