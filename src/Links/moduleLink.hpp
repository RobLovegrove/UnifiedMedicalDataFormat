#ifndef MODULE_LINK_HPP
#define MODULE_LINK_HPP


#include <string>
#include "../Utility/uuid.hpp"
#include "../DataModule/ModuleData.hpp"
#include "../Utility/moduleType.hpp"

struct Encounter {
    UUID encounterId;
    std::optional<UUID> rootModule;
    std::optional<UUID> lastModule;
};

enum class ModuleLinkType {
    BELONGS_TO,
    VARIANT_OF,
    ANNOTATES
};

struct ModuleLink {
    UUID sourceId;
    UUID targetId;
    ModuleLinkType linkType;
    bool deleted = false;

    ModuleLink(UUID sourceId, UUID targetId, ModuleLinkType linkType) : sourceId(sourceId), targetId(targetId), linkType(linkType) {}

    bool operator==(const ModuleLink& other) const {
        return sourceId == other.sourceId &&
               targetId == other.targetId &&
               linkType == other.linkType;
    }
};

#endif // MODULE_LINK_HPP