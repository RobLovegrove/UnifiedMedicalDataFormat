#ifndef XREF_HPP
#define XREF_HPP

#include "utils.hpp"

#include <fstream>
#include <iostream>

struct XrefEntry {
    uint32_t id;
    uint8_t type;
    uint32_t size;
    uint64_t offset;
};

class XRefTable {
private:
    std::vector<XrefEntry> entries;
    uint32_t nextId = 0;
    uint64_t xrefOffset;

    static char xrefMarker[12];
    static char EOFmarker[8];

    //std::map<int, std::streampos> references;

public:
    uint32_t addEntry(ModuleType type, uint64_t offset, uint32_t size);
    bool deleteEntry(uint32_t entryId);
    const XrefEntry* findEntry(ModuleType);

    void setXrefOffset(uint64_t offset) { xrefOffset = offset; }
    bool writeXref(std::ostream& out) const;

    static XRefTable loadXrefTable(std::ifstream& in);

};

#endif