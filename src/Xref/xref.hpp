#ifndef XREF_HPP
#define XREF_HPP

#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "DataModule/moduleType.hpp"

#include <fstream>
#include <iostream>
#include <array>

struct XrefEntry {
    UUID id;
    uint8_t type;
    uint32_t size;
    uint64_t offset;
};

class XRefTable {
private:
    std::vector<XrefEntry> entries;
    uint64_t xrefOffset;

    static char xrefMarker[12];
    static char EOFmarker[8];

    //std::map<int, std::streampos> references;

public:
    void addEntry(ModuleType type, UUID uuid, uint64_t offset, uint32_t size);
    bool deleteEntry(UUID entryId);
    const XrefEntry* findEntry(ModuleType);

    void setXrefOffset(uint64_t offset) { xrefOffset = offset; }
    bool writeXref(std::ostream& out) const;

    static XRefTable loadXrefTable(std::ifstream& in);

};

#endif