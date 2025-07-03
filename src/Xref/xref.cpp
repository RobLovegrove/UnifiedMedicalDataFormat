#include "xref.hpp"
#include "Utility/utils.hpp"

#include <string>
#include <algorithm> 
#include <vector>
#include <fstream>

using namespace std;

char XRefTable::xrefMarker[12] = "xrefoffset\n";
char XRefTable::EOFmarker[8] = "#EOUMDF";

uint32_t XRefTable::addEntry(ModuleType type, uint64_t offset, uint32_t size) {
    XrefEntry newEntry;
    newEntry.id = nextId++;
    newEntry.type = static_cast<uint8_t>(type);
    newEntry.offset = offset;
    newEntry.size = size;
    
    entries.push_back(newEntry);
    return newEntry.id;
}

bool XRefTable::deleteEntry(uint32_t entryId) {

    if (erase_if(entries, [entryId](const XrefEntry& entry) {
        return entry.id == entryId;
    }) > 0) { return true; }
    else { return false; }
    
}

const XrefEntry* XRefTable::findEntry(ModuleType type) {
    for (const XrefEntry& entry : entries) {
        if (entry.type == static_cast<uint8_t>(type)) return &entry;
    }
    return nullptr;
}

bool XRefTable::writeXref(std::ostream& out) const{
    // 1. Write Header Signature
    out.write("XREF", 4);

    // 2. Write Entry Count
    uint32_t count = static_cast<uint32_t>(entries.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // 3. Write field widths: [id=4, type=1, size=4, offset=8]
    uint8_t widths[4] = {4, 1, 4, 8};
    out.write(reinterpret_cast<const char*>(widths), sizeof(widths));

    // 4. Write Reserved (zeroed)
    uint8_t reserved[32] = {};
    out.write(reinterpret_cast<const char*>(reserved), sizeof(reserved));

    // 5. Write Each Entry as binary row
    for (const auto& entry : entries) {
        out.write(reinterpret_cast<const char*>(&entry.id), sizeof(entry.id));
        out.write(reinterpret_cast<const char*>(&entry.type), sizeof(entry.type));
        out.write(reinterpret_cast<const char*>(&entry.size), sizeof(entry.size));
        out.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
    }

    // 6. Write footer
    out.write(xrefMarker, sizeof(xrefMarker));
    out.write(reinterpret_cast<const char*>(&xrefOffset), sizeof(xrefOffset));

    out.write(EOFmarker, sizeof(EOFmarker));

    return out.good();
}

XRefTable XRefTable::loadXrefTable(std::ifstream& in) {

    constexpr size_t FOOTER_SIZE = sizeof(EOFmarker) + sizeof(xrefOffset) + sizeof(xrefMarker);

    XRefTable table;

    // 1. Seek to end minus footer size
    in.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(in.tellg());
    
    if (fileSize < FOOTER_SIZE) {
        throw runtime_error("File too small to contain valid footer.");
    }

    in.seekg(fileSize - FOOTER_SIZE);

    // 2. Read footer marker and xref offset
    char inXrefMarker[8];
    uint64_t inXrefOffset = 0;
    char inEOFmarker[8];

    in.read(inXrefMarker, sizeof(xrefMarker));
    in.read(reinterpret_cast<char*>(&inXrefOffset), sizeof(xrefOffset));
    in.read(inEOFmarker, sizeof(EOFmarker));

    if (std::memcmp(inXrefMarker, xrefMarker, 8) != 0) {
        throw std::runtime_error("Invalid Xref offset marker.");
    }
    if (std::memcmp(inEOFmarker, EOFmarker, 8) != 0) {
        throw std::runtime_error("Invalid footer marker.");
    }

    // 3. Seek to xref offset
    in.seekg(inXrefOffset);

    // 4. Read "XREF" signature
    char xrefSig[4];
    in.read(xrefSig, 4);
    if (std::memcmp(xrefSig, "XREF", 4) != 0) {
        throw std::runtime_error("Missing XREF signature.");
    }

    // 5. Read entry count
    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    // 6. Read widths
    uint8_t widths[4];
    in.read(reinterpret_cast<char*>(widths), sizeof(widths));

    // 7. Skip reserved
    uint8_t reserved[32];
    in.read(reinterpret_cast<char*>(reserved), sizeof(reserved));

    // 8. Read entries
    for (uint32_t i = 0; i < count; ++i) {
        XrefEntry entry{};
        in.read(reinterpret_cast<char*>(&entry.id), sizeof(entry.id));
        in.read(reinterpret_cast<char*>(&entry.type), sizeof(entry.type));
        in.read(reinterpret_cast<char*>(&entry.size), sizeof(entry.size));
        in.read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
        table.entries.push_back(entry);
    }

    return table;
}