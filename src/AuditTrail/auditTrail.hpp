struct ModuleTrail {
    uint64_t moduleOffset;
    bool isCurrent;
    DateTime createdAt;
    DateTime modifiedAt;
    std::string createdBy;
    std::string modifiedBy;
}

std::vector<ModuleTrail> getAuditTrail(const UUID& moduleId);