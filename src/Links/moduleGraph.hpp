#ifndef MODULE_GRAPH_HPP
#define MODULE_GRAPH_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "moduleLink.hpp"
#include "../Utility/uuid.hpp"
#include <expected>
#include <nlohmann/json.hpp>

class ModuleGraph {
private:
    std::vector<std::shared_ptr<ModuleLink>> links;

    std::unordered_map<UUID, std::vector<std::shared_ptr<ModuleLink>>> adjacency;        
    std::unordered_map<UUID, std::vector<std::shared_ptr<ModuleLink>>> reverseAdjacency;

    // Encounter ID → root module(s)
    std::unordered_map<UUID, Encounter> encounters;

    std::streampos encounterSizeOffset;
    std::streampos linkSizeOffset;

    uint32_t encounterSize = 0;
    uint32_t linkSize = 0;

    bool wouldCreateCycle(const UUID& source, const UUID& target) const;

    // Writing Methods
    void writeGraphHeader(std::ostream& outfile);
    void writeEncounters(std::ostream& outfile);
    void writeLinks(std::ostream& outfile);
    void updateHeader(std::ostream& outfile);

    // Reading Methods
    void readGraphHeader(std::istream& in);
    void readEncounters(std::istream& in);
    void readLinks(std::istream& in);
    void buildAdjacencyLists();

    bool hasCycle() const;

    // JSON building helper methods
    nlohmann::json buildEncounterModuleChain(const Encounter& encounter) const;
    nlohmann::json buildModuleWithDerivedData(const UUID& moduleId) const;
    nlohmann::json buildGraphSummary() const;

public:

    static ModuleGraph readModuleGraph(std::istream& in);

    void addLink(const ModuleLink& link);
    void removeLink(const ModuleLink& link);

    const std::vector<std::shared_ptr<ModuleLink>>& getOutgoingLinks(const UUID& moduleId) const;
    const std::vector<std::shared_ptr<ModuleLink>>& getIncomingLinks(const UUID& moduleId) const;
    const std::vector<std::shared_ptr<ModuleLink>>& allLinks() const;

    const std::unordered_map<UUID, Encounter>& getEncounters() const { return encounters; }
    UUID createEncounter();

    bool encounterExists(const UUID& encounterId) const;
    Encounter& getEncounter(const UUID& encounterId);

    void addModuleToEncounter(const UUID& encounterId, const UUID& moduleId);
    void removeModuleFromEncounter(const UUID& encounterId, const UUID& moduleId);

    void addModuleLink(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType);
    void removeModuleLink(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType);

    // Graph traversal methods
    std::vector<UUID> getRootModules() const;

    uint32_t writeModuleGraph(std::ostream& outfile);
    
    void displayLinks() const;
    void displayEncounters() const;
    void printEncounterPath(const UUID& encounterId) const;

    // JSON export method
    nlohmann::json toJson() const;

    // std::vector<UUID> getEncounterMembers(const UUID& rootId) const;
    // std::vector<UUID> getDerivedData(const UUID& sourceId) const;
    // std::vector<UUID> getAnnotations(const UUID& sourceId) const;
};


#endif // MODULE_GRAPH_HPP