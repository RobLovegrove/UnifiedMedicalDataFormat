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

    uint64_t moduleGraphOffset;
    uint32_t moduleGraphSize;

    static char xrefMarker[12];
    static char EOFmarker[8];

    //std::map<int, std::streampos> references;

public:
    void addEntry(ModuleType type, UUID uuid, uint64_t offset, uint32_t size);
    bool deleteEntry(UUID entryId);
    void clear() { entries.clear(); }

    // const XrefEntry* findEntry(ModuleType);
    const XrefEntry& getEntry(UUID id) const;

    std::vector<XrefEntry>& getEntries() { return entries; }

    bool contains(UUID id) const;

    void setXrefOffset(uint64_t offset) { xrefOffset = offset; }
    uint64_t getXrefOffset() const { return xrefOffset; }

    void setModuleGraphOffset(uint64_t offset) { moduleGraphOffset = offset; }
    uint64_t getModuleGraphOffset() const { return moduleGraphOffset; }

    void setModuleGraphSize(uint32_t size) { moduleGraphSize = size; }
    uint32_t getModuleGraphSize() const { return moduleGraphSize; }

    bool writeXref(std::ostream& out) const;

    static XRefTable loadXrefTable(std::istream& in);

    friend std::ostream& operator<<(std::ostream& os, const XRefTable& table);

    void setObsolete(std::ostream& out);

    void updateEntryOffset(UUID id, uint64_t offset);

};

#endif