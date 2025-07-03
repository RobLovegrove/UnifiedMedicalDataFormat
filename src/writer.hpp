#ifndef WRITER_HPP
#define WRITER_HPP

#include "Header/header.hpp"
#include "Patient/patient.hpp"
#include "Xref/xref.hpp"

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
    Patient patient;
    XRefTable xref;

    bool writeHeader(std::ofstream& outfile);
    bool writePatientData(std::ofstream& outfile);
    bool writeXref(std::ofstream& outfile);

public:

    // Returns true on success, false on failure.
    bool writeNewFile(const std::string& filename, const Patient patient, const std::string& patientData);
    void setFileAccessMode(FileAccessMode);
    FileAccessMode getFileAccessMode() const { return accessMode; }

};

#endif