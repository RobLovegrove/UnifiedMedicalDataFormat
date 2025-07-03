#ifndef HEADER_HPP
#define HEADER_HPP

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

#include "Utility/utils.hpp"
#include "Xref/xref.hpp"

class Header {

private:
    // Define a magic number and version
    inline static constexpr Version UMDF_VERSION = Version{1, 0, 0};
    inline static constexpr std::string_view MAGIC_NUMBER = "#UMDFv1.0\n";

public:
    bool writePrimaryHeader(std::ofstream& outfile, XRefTable& xref);
    bool readPrimaryHeader(std::ifstream& inFile);

    static bool writeDataHeader(std::ofstream& outfile, ModuleType module, XRefTable& xref);
    static bool readDataHeader(std::ofstream& outfile);
};

#endif