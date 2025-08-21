#ifndef HEADER_HPP
#define HEADER_HPP

#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <istream>

#include "Utility/utils.hpp"
#include "Xref/xref.hpp"
#include "DataModule/Header/dataHeader.hpp"

class Header {

private:
    // Define a magic number and version
    inline static constexpr Version UMDF_VERSION = Version{1, 0, 0};
    inline static constexpr std::string_view MAGIC_NUMBER = "#UMDFv1.0\n";

    EncryptionData encryptionData;

    std::streampos writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const;

public:
    bool writePrimaryHeader(std::ostream& outfile);
    std::expected<EncryptionData, std::string> readPrimaryHeader(std::istream& inFile);

    void setEncryptionData(EncryptionData data) { encryptionData = data; }
    EncryptionData getEncryptionData() const { return encryptionData; }
};

#endif