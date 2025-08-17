#ifndef XREF_HPP
#define XREF_HPP

#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "Utility/moduleType.hpp"

#include <fstream>
#include <iostream>
#include <array>
#include <vector>

struct XrefEntry {
    UUID id;
    uint8_t type;
    uint64_t size;
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
    void clear() { entries.clear(); }

    const XrefEntry* findEntry(ModuleType);
    std::vector<XrefEntry>& getEntries() { return entries; }

    void setXrefOffset(uint64_t offset) { xrefOffset = offset; }
    uint64_t getXrefOffset() const { return xrefOffset; }

    bool writeXref(std::ostream& out) const;

    static XRefTable loadXrefTable(std::istream& in);

    friend std::ostream& operator<<(std::ostream& os, const XRefTable& table);

    void setObsolete(std::ostream& out);

    void updateEntryOffset(UUID id, uint64_t offset);

};

#endif