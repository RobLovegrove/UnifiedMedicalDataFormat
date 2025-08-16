#include "xref.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

#include <string>
#include <algorithm> 
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

char XRefTable::xrefMarker[12] = "xrefoffset\n";
char XRefTable::EOFmarker[8] = "#EOUMDF";

void XRefTable::addEntry(ModuleType type, UUID uuid, uint64_t offset, uint32_t size) {
    XrefEntry newEntry;
    newEntry.id = uuid;
    newEntry.type = static_cast<uint8_t>(type);
    newEntry.offset = offset;
    newEntry.size = size;
    
    entries.push_back(newEntry);
}

bool XRefTable::deleteEntry(UUID entryId) {

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

    // 2. Write Current Table Flag
    uint8_t isCurrent = 1;  // 1 = current, 0 = obsolete
    out.write(reinterpret_cast<const char*>(&isCurrent), sizeof(isCurrent));

    // 3. Write Entry Count
    uint32_t count = static_cast<uint32_t>(entries.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // 4. Write field widths: [id=16, type=1, size=4, offset=8]
    uint8_t widths[4] = {16, 1, 8, 8};
    out.write(reinterpret_cast<const char*>(widths), sizeof(widths));

    // 5. Write Reserved (zeroed)
    uint8_t reserved[32] = {};
    out.write(reinterpret_cast<const char*>(reserved), sizeof(reserved));

    // 6. Write Each Entry as binary row
    for (const auto& entry : entries) {
        out.write(reinterpret_cast<const char*>(&entry.id), sizeof(entry.id));
        out.write(reinterpret_cast<const char*>(&entry.type), sizeof(entry.type));
        out.write(reinterpret_cast<const char*>(&entry.size), sizeof(entry.size));
        out.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
    }

    // 7. Write footer
    out.write(xrefMarker, sizeof(xrefMarker));
    out.write(reinterpret_cast<const char*>(&xrefOffset), sizeof(xrefOffset));

    out.write(EOFmarker, sizeof(EOFmarker));

    return out.good();
}

XRefTable XRefTable::loadXrefTable(std::istream& in) {

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
    char inXrefMarker[12];
    uint64_t inXrefOffset = 0;
    char inEOFmarker[8];

    in.read(inXrefMarker, sizeof(xrefMarker));
    in.read(reinterpret_cast<char*>(&inXrefOffset), sizeof(xrefOffset));
    in.read(inEOFmarker, sizeof(EOFmarker));

    if (std::memcmp(inXrefMarker, xrefMarker, 12) != 0) {
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

    // 5. Read Current Table Flag
    uint8_t isCurrent = 0;
    in.read(reinterpret_cast<char*>(&isCurrent), sizeof(isCurrent));
    if (isCurrent == 0) {
        throw std::runtime_error("Obsolete Xref table.");
    }

    // 6. Read entry count
    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    // 7. Read widths
    uint8_t widths[4];
    in.read(reinterpret_cast<char*>(widths), sizeof(widths));

    if (widths[0] != 16 || widths[1] != 1 || widths[2] != 8 || widths[3] != 8) {
        throw std::runtime_error("Unexpected field widths.");
    }

    // 8. Skip reserved
    uint8_t reserved[32];
    in.read(reinterpret_cast<char*>(reserved), sizeof(reserved));

    // 9. Read entries
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

ostream& operator<<(ostream& os, const XRefTable& table) {
    os << "XRefTable (" << table.entries.size() << " entries):\n";

    for (const auto& entry : table.entries) {
        os << "  UUID: " << entry.id.toString()
           << " | Type: " << static_cast<ModuleType>(entry.type)
           << " | Size: " << entry.size
           << " | Offset: 0x" << std::hex << std::setw(16) << std::setfill('0') << entry.offset
           << std::dec << '\n';
    }

    os << "  Xref Offset: 0x" << std::hex << table.xrefOffset << std::dec << '\n';
    return os;
}