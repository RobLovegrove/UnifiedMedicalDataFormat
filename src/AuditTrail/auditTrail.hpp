#ifndef AUDIT_TRAIL_HPP
#define AUDIT_TRAIL_HPP

#include <vector>
#include <string>
#include <fstream>

#include "../Utility/uuid.hpp"
#include "../Utility/dateTime.hpp"
#include "../Xref/xref.hpp"
#include "../DataModule/Header/dataHeader.hpp"

struct ModuleTrail {
    uint64_t moduleOffset;
    bool isCurrent;
    DateTime createdAt;
    DateTime modifiedAt;
    std::string createdBy;
    std::string modifiedBy;
    uint64_t moduleSize;
    ModuleType moduleType;
};

class AuditTrail {
    private:
    UUID initialModuleID;
    std::vector<ModuleTrail> auditTrail;

    void recursiveTrail(std::istream& auditTrailFile, uint64_t offset);

    public:
        AuditTrail(UUID initialModuleID, std::istream& auditTrailFile, XRefTable& xrefTable);

        std::vector<ModuleTrail> getModuleTrail() const {
            return auditTrail;
        }

        UUID getInitialModuleID() const { return initialModuleID; }

};

#endif