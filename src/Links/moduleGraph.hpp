#ifndef MODULE_GRAPH_HPP
#define MODULE_GRAPH_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "moduleLink.hpp"
#include "../Utility/uuid.hpp"
#include <expected>


class ModuleGraph {
private:
    std::vector<std::shared_ptr<ModuleLink>> links;

    std::unordered_map<UUID, std::vector<std::shared_ptr<ModuleLink>>> adjacency;        
    std::unordered_map<UUID, std::vector<std::shared_ptr<ModuleLink>>> reverseAdjacency;

    // Encounter ID → root module(s)
    std::unordered_map<UUID, Encounter> encounters;


    std::streampos encounterSizeOffset;
    std::streampos linkSizeOffset;

    uint32_t encounterSize;
    uint32_t linkSize;

    bool wouldCreateCycle(const UUID& source, const UUID& target) const;

    // Writing Methods
    void writeGraphHeader(std::ostream& outfile);
    void writeEncounters(std::ostream& outfile);
    void writeLinks(std::ostream& outfile);
    void updateHeader(std::ostream& outfile);

public:
    void addLink(const ModuleLink& link);
    void removeLink(const ModuleLink& link);

    const std::vector<std::shared_ptr<ModuleLink>>& getOutgoingLinks(const UUID& moduleId) const;
    const std::vector<std::shared_ptr<ModuleLink>>& getIncomingLinks(const UUID& moduleId) const;
    const std::vector<std::shared_ptr<ModuleLink>>& allLinks() const;


    UUID createEncounter();

    bool encounterExists(const UUID& encounterId) const;
    Encounter& getEncounter(const UUID& encounterId);

    void addModuleToEncounter(const UUID& encounterId, const UUID& moduleId);
    void removeModuleFromEncounter(const UUID& encounterId, const UUID& moduleId);

    void addLinkModule(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType);
    void removeLinkModule(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType);

    // Graph traversal methods
    std::vector<UUID> getRootModules() const;

    void writeModuleGraph(std::ostream& outfile);

    // std::vector<UUID> getEncounterMembers(const UUID& rootId) const;
    // std::vector<UUID> getDerivedData(const UUID& sourceId) const;
    // std::vector<UUID> getAnnotations(const UUID& sourceId) const;
};


#endif // MODULE_GRAPH_HPP