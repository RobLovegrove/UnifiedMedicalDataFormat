#include <vector>
#include <string>
#include <fstream>

#include "../Utility/uuid.hpp"
#include "../Utility/dateTime.hpp"
#include "../Xref/xref.hpp"

struct ModuleTrail {
    uint64_t moduleOffset;
    bool isCurrent;
    DateTime createdAt;
    DateTime modifiedAt;
    std::string createdBy;
    std::string modifiedBy;
};

class AuditTrail {
    private:
    UUID initialModuleID;
    std::vector<ModuleTrail> auditTrail;

    public:
        AuditTrail(UUID initialModuleID, std::istream& auditTrailFile, XRefTable& xrefTable);

        std::vector<ModuleTrail> getModuleTrail() const {
            return auditTrail;
        }

        UUID getInitialModuleID() const { return initialModuleID; }

};