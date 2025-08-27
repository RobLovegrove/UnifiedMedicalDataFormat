#ifndef AUDIT_TRAIL_HPP
#define AUDIT_TRAIL_HPP

#include <vector>
#include <string>
#include <fstream>
#include <memory>

#include "../Utility/uuid.hpp"
#include "../Utility/dateTime.hpp"
#include "../Xref/xref.hpp"
#include "../DataModule/Header/dataHeader.hpp"
#include "../DataModule/dataModule.hpp"

struct ModuleTrail {
    uint64_t moduleOffset;
    bool isCurrent;
    DateTime createdAt;
    DateTime modifiedAt;
    std::string createdBy;
    std::string modifiedBy;
    uint64_t moduleSize;
    ModuleType moduleType;
    UUID moduleID;
};

class AuditTrail {
    private:
    UUID initialModuleID;
    std::vector<ModuleTrail> auditTrail;

    std::vector<std::unique_ptr<DataModule>> loadedModules;

    void recursiveTrail(std::istream& auditTrailFile, uint64_t offset);

    public:
        AuditTrail(UUID initialModuleID, std::istream& auditTrailFile, XRefTable& xrefTable);

        std::vector<ModuleTrail> getModuleTrail() const {
            return auditTrail;
        }

        UUID getInitialModuleID() const { return initialModuleID; }

        std::optional<ModuleData> getModuleData(const UUID& moduleId);

        void addLoadedModule(std::unique_ptr<DataModule> module) {
            loadedModules.push_back(std::move(module));
        }

};

#endif