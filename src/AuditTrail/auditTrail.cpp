#include "auditTrail.hpp"
#include "../Xref/xref.hpp"
#include "../DataModule/Header/dataHeader.hpp"

#include <iostream>

AuditTrail::AuditTrail(UUID initialModuleID, 
    std::istream& auditTrailFile, XRefTable& xrefTable) : initialModuleID(initialModuleID) {

        ModuleTrail moduleTrail;

        // Get the initial module offset
        XrefEntry initialModule = xrefTable.getEntry(initialModuleID);
        uint64_t initialModuleOffset = initialModule.offset;

        recursiveTrail(auditTrailFile, initialModuleOffset);

}

void AuditTrail::recursiveTrail(std::istream& auditTrailFile, uint64_t offset) {

    if (offset == 0) {
        return;
    }

    // go to previous offset
    auditTrailFile.seekg(offset);

    // read the module header
    DataHeader moduleHeader;
    moduleHeader.readDataHeader(auditTrailFile);

    if (moduleHeader.getModuleID() != initialModuleID) {
        throw std::runtime_error("Module ID mismatch when reading audit trail");
    }

    ModuleTrail moduleTrail;
    moduleTrail.moduleOffset = offset;
    moduleTrail.isCurrent = moduleHeader.getIsCurrent();
    moduleTrail.createdAt = moduleHeader.getCreatedAt();
    moduleTrail.modifiedAt = moduleHeader.getModifiedAt();
    moduleTrail.createdBy = moduleHeader.getCreatedBy();
    moduleTrail.modifiedBy = moduleHeader.getModifiedBy();
    moduleTrail.moduleType = moduleHeader.getModuleType();
    moduleTrail.moduleSize = (
        moduleHeader.getHeaderSize() 
        + moduleHeader.getStringBufferSize()
        + moduleHeader.getMetadataSize() 
        + moduleHeader.getDataSize()
    );
    auditTrail.push_back(moduleTrail);

    uint64_t nextOffset = moduleHeader.getPrevious();
    recursiveTrail(auditTrailFile, nextOffset);
}