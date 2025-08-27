#include "auditTrail.hpp"
#include "../Xref/xref.hpp"

#include <iostream>

AuditTrail::AuditTrail(UUID initialModuleID, 
    std::istream& auditTrailFile, XRefTable& xrefTable) {

        ModuleTrail moduleTrail;

        // Get the initial module offset
        XrefEntry initialModule = xrefTable.findEntry(initialModuleID);
        uint64_t initialModuleOffset = initialModule.offset;

        // Go to initial module start
        uint64_t previousOffset = 0

        // Read the module header and populate ModuleTrail 
        // Keep previousOffset for next module Trail
        // Add module Trail to vector

        // Go to next module start


}