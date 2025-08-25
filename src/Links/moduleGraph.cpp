#include "moduleGraph.hpp"
#include "../Utility/tlvHeader.hpp"

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <stack>
#include <sstream>




using namespace std;

// Helper functions

bool ModuleGraph::wouldCreateCycle(const UUID& source, const UUID& target) const {
    std::unordered_set<UUID> visited;
    std::stack<UUID> stack;
    stack.push(target); // start from the new target

    // DFS to check for cycles
    while (!stack.empty()) {
        UUID current = stack.top();
        stack.pop();

        if (current == source) {
            return true; // cycle detected
        }

        if (!visited.insert(current).second) {
            continue; // already visited
        }

        auto it = adjacency.find(current);
        if (it != adjacency.end()) {
            for (const auto& link : it->second) {
                stack.push(link->targetId);
            }
        }
    }
    return false;
}

UUID ModuleGraph::createEncounter() {
    UUID encounterId = UUID();  // generates a new UUID
    Encounter encounter;
    encounter.encounterId = encounterId;
    encounters[encounterId] = encounter;
    return encounterId;
}

bool ModuleGraph::encounterExists(const UUID& encounterId) const {
    return encounters.contains(encounterId);
}

Encounter& ModuleGraph::getEncounter(const UUID& encounterId) {
    auto it = encounters.find(encounterId);
    if (it == encounters.end()) {
        throw std::runtime_error("Encounter ID not found");
    }
    return it->second;
}

void ModuleGraph::addModuleToEncounter(const UUID& encounterId, const UUID& moduleId) {
    auto encounter = getEncounter(encounterId);
    if (!encounter.rootModule.has_value()) {
        // First module becomes the root
        encounter.rootModule = moduleId;
        encounter.lastModule = moduleId;
    } else {
        // Link to the last module added
        ModuleLink link(encounter.lastModule.value(), moduleId, ModuleLinkType::BELONGS_TO);
        addLink(link);

        encounter.lastModule = moduleId; // update the last module
    }
}

void ModuleGraph::removeModuleFromEncounter(const UUID& encounterId, const UUID& moduleId) {
    Encounter& encounter = getEncounter(encounterId);

    // Remove links where this module is source or target
    auto outgoingIt = adjacency.find(moduleId);
    if (outgoingIt != adjacency.end()) {
        for (auto& link : outgoingIt->second) {
            // Remove from flat links vector
            links.erase(std::remove(links.begin(), links.end(), link), links.end());

            // Remove reverse adjacency entry
            auto revIt = reverseAdjacency.find(link->targetId);
            if (revIt != reverseAdjacency.end()) {
                revIt->second.erase(std::remove(revIt->second.begin(), revIt->second.end(), link), revIt->second.end());
            }
        }
        adjacency.erase(outgoingIt);
    }

    auto incomingIt = reverseAdjacency.find(moduleId);
    if (incomingIt != reverseAdjacency.end()) {
        for (auto& link : incomingIt->second) {
            // Remove from flat links vector
            links.erase(std::remove(links.begin(), links.end(), link), links.end());

            // Remove from adjacency
            auto adjIt = adjacency.find(link->sourceId);
            if (adjIt != adjacency.end()) {
                adjIt->second.erase(std::remove(adjIt->second.begin(), adjIt->second.end(), link), adjIt->second.end());
            }
        }
        reverseAdjacency.erase(incomingIt);
    }

    // Update rootModule / lastModule if necessary
    if (encounter.rootModule.value() == moduleId) {
        encounter.rootModule = std::nullopt;  // empty UUID
        encounter.lastModule = std::nullopt;
    } else if (encounter.lastModule == moduleId) {
        // Find new lastModule (optional: could traverse adjacency to find the previous module)
        encounter.lastModule = encounter.rootModule; // simple fallback
    }
}


void ModuleGraph::addLinkModule(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType) {

    ModuleLink link(parentModuleId, moduleId, linkType);
    addLink(link);
}

void ModuleGraph::removeLinkModule(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType) {

    ModuleLink link(parentModuleId, moduleId, linkType);
    removeLink(link);
}


// ModuleGraph methods
void ModuleGraph::addLink(const ModuleLink& link) {
    // Check for cycle
    if (wouldCreateCycle(link.sourceId, link.targetId)) {
        throw std::runtime_error("Adding this link would create a cycle!");
    }

    // No cycle, add link
    auto linkPtr = std::make_shared<ModuleLink>(link);
    links.push_back(linkPtr);
    adjacency[link.sourceId].push_back(linkPtr);
    reverseAdjacency[link.targetId].push_back(linkPtr);
}

void ModuleGraph::removeLink(const ModuleLink& link) {

    // Remove from flat list
    links.erase(
        std::remove_if(links.begin(), links.end(),
            [&](const auto& linkPtr) {
                return *linkPtr == link;
            }),
        links.end());

    // Remove from adjacency
    auto itAdj = adjacency.find(link.sourceId);
    if (itAdj != adjacency.end()) {
        auto& outLinks = itAdj->second;
        outLinks.erase(
            std::remove_if(outLinks.begin(), outLinks.end(),
                [&](const auto& linkPtr) { return *linkPtr == link; }),
            outLinks.end());
    }

    // Remove from reverse adjacency
    auto itRev = reverseAdjacency.find(link.targetId);
    if (itRev != reverseAdjacency.end()) {
        auto& inLinks = itRev->second;
        inLinks.erase(
            std::remove_if(inLinks.begin(), inLinks.end(),
                [&](const auto& linkPtr) { return *linkPtr == link; }),
            inLinks.end());
    }
}




void ModuleGraph::writeModuleGraph(std::ostream& outfile) {

    // Create a buffer to write to
    std::stringstream buffer;

    // Write graph header
    try {
        writeGraphHeader(buffer);
    } catch (const std::exception& e) {
        throw std::runtime_error("Exception writing graph header: " + std::string(e.what()));
    }

    // Write encounters
    try {
        writeEncounters(buffer);
    } catch (const std::exception& e) {
        throw std::runtime_error("Exception writing encounters: " + std::string(e.what()));
    }

    // Write links
    try {
        writeLinks(buffer);
    } catch (const std::exception& e) {
        throw std::runtime_error("Exception writing links: " + std::string(e.what()));
    }

    outfile << buffer.str();
}


void ModuleGraph::writeGraphHeader(std::ostream& outfile) {
    // Get start position
    std::streampos startPos = outfile.tellp();

    // WRITE HEADER SIZE
    uint32_t headerSize = 0;
    streampos headerSizeOffset = writeTLVFixed(
        outfile, 
        HeaderFieldType::HeaderSize, 
        &headerSize, 
        sizeof(uint32_t));

    // WRITE ENCOUNTER SIZE
    encounterSizeOffset = writeTLVFixed(
        outfile, 
        HeaderFieldType::EncounterSize, 
        &encounterSize, 
        sizeof(encounterSize));

    // WRITE LINK SIZE
    linkSizeOffset = writeTLVFixed(
        outfile, 
        HeaderFieldType::LinkSize, 
        &linkSize, 
        sizeof(linkSize));

    // Update the header size
    std::streampos currentPos = outfile.tellp();
    headerSize = static_cast<uint32_t>(currentPos - startPos);
    outfile.seekp(headerSizeOffset);
    outfile.write(reinterpret_cast<const char*>(&headerSize), sizeof(uint32_t));

    // Restore the position
    outfile.seekp(currentPos);
}

void ModuleGraph::updateHeader(std::ostream& outfile) {

    streampos currentPos = outfile.tellp();

    outfile.seekp(encounterSizeOffset);
    outfile.write(reinterpret_cast<const char*>(&encounterSize), sizeof(encounterSize));

    outfile.seekp(linkSizeOffset);
    outfile.write(reinterpret_cast<const char*>(&linkSize), sizeof(linkSize));

    outfile.seekp(currentPos);
}

void ModuleGraph::writeEncounters(std::ostream& outfile) {
    for (auto& [encounterId, encounter] : encounters) {
        if (encounter.rootModule.has_value()) {

            std::string encounterIdStr = encounterId.toString();
            std::string rootModuleStr = encounter.rootModule.value().toString();
            std::string lastModuleStr = encounter.lastModule.value().toString();

            outfile.write(reinterpret_cast<const char*>(encounterIdStr.data()), sizeof(encounterIdStr.size()));
            outfile.write(reinterpret_cast<const char*>(rootModuleStr.data()), sizeof(rootModuleStr.size()));
            if (encounter.lastModule.has_value()) {
                outfile.write(reinterpret_cast<const char*>(lastModuleStr.data()), sizeof(lastModuleStr.size()));
            }
            else {
                outfile.write(reinterpret_cast<const char*>(encounterIdStr.data()), sizeof(encounterIdStr.size()));
            }
        }
    }
}

void ModuleGraph::writeLinks(std::ostream& outfile) {
    for (auto& linkPtr : links) {
        outfile.write(reinterpret_cast<const char*>(&linkPtr->sourceId), sizeof(linkPtr->sourceId));
        outfile.write(reinterpret_cast<const char*>(&linkPtr->targetId), sizeof(linkPtr->targetId));
        outfile.write(reinterpret_cast<const char*>(&linkPtr->linkType), sizeof(linkPtr->linkType));
    }
}





const std::vector<std::shared_ptr<ModuleLink>>& ModuleGraph::getOutgoingLinks(const UUID& moduleId) const {
    static const std::vector<std::shared_ptr<ModuleLink>> empty;
    auto it = adjacency.find(moduleId);
    return (it != adjacency.end()) ? it->second : empty;
}

const std::vector<std::shared_ptr<ModuleLink>>& ModuleGraph::getIncomingLinks(const UUID& moduleId) const {
    static const std::vector<std::shared_ptr<ModuleLink>> empty;
    auto it = reverseAdjacency.find(moduleId);
    return (it != reverseAdjacency.end()) ? it->second : empty;
}

const std::vector<std::shared_ptr<ModuleLink>>& ModuleGraph::allLinks() const {
    return links;
}

std::vector<UUID> ModuleGraph::getRootModules() const {
    std::vector<UUID> roots;
    for (const auto& [moduleId, linksIn] : reverseAdjacency) {
        bool hasParent = false;
        for (const auto& link : linksIn) {
            if (link->linkType == ModuleLinkType::BELONGS_TO) {
                hasParent = true;
                break;
            }
        }
        if (!hasParent) {
            roots.push_back(moduleId);
        }
    }
    return roots;
}

// std::vector<UUID> ModuleGraph::getEncounterMembers(const UUID& rootId) const {
//     std::vector<UUID> members;
//     std::unordered_set<UUID> visited;
//     std::queue<UUID> toVisit;
    
//     // Start with the root
//     toVisit.push(rootId);
//     visited.insert(rootId);
    
//     while (!toVisit.empty()) {
//         UUID current = toVisit.front();
//         toVisit.pop();
//         members.push_back(current);
        
//         // Find all modules that belong to the current module
//         // Look for incoming BELONGS_TO links TO the current module
//         auto reverseIt = reverseAdjacency.find(current);
//         if (reverseIt != reverseAdjacency.end()) {
//             for (const auto& link : reverseIt->second) {
//                 if (link->linkType == ModuleLinkType::BELONGS_TO && !link->deleted) {
//                     UUID source = link->sourceId;  // The module that belongs to current
//                     if (visited.find(source) == visited.end()) {
//                         toVisit.push(source);
//                         visited.insert(source);
//                     }
//                 }
//             }
//         }
        
//         // Also find modules that the current module belongs to
//         // Look for outgoing BELONGS_TO links FROM the current module
//         auto it = adjacency.find(current);
//         if (it != adjacency.end()) {
//             for (const auto& link : it->second) {
//                 if (link->linkType == ModuleLinkType::BELONGS_TO && !link->deleted) {
//                     UUID target = link->targetId;  // The module that current belongs to
//                     if (visited.find(target) == visited.end()) {
//                         toVisit.push(target);
//                         visited.insert(target);
//                     }
//                 }
//             }
//         }
//     }
    
//     return members;
// }

// std::vector<UUID> ModuleGraph::getDerivedData(const UUID& sourceId) const {
//     std::vector<UUID> derived;
//     std::unordered_set<UUID> visited;
//     std::queue<UUID> toVisit;
    
//     // Start with the source
//     toVisit.push(sourceId);
//     visited.insert(sourceId);
    
//     while (!toVisit.empty()) {
//         UUID current = toVisit.front();
//         toVisit.pop();
        
//         // Find all modules that are derived from the current module
//         // Look for incoming DERIVED_FROM links TO the current module
//         auto reverseIt = reverseAdjacency.find(current);
//         if (reverseIt != reverseAdjacency.end()) {
//             for (const auto& link : reverseIt->second) {
//                 if (link->linkType == ModuleLinkType::DERIVED_FROM && !link->deleted) {
//                     UUID source = link->sourceId;  // The module that is derived from current
//                     if (visited.find(source) == visited.end()) {
//                         derived.push_back(source);
//                         toVisit.push(source);
//                         visited.insert(source);
//                     }
//                 }
//             }
//         }
//     }
    
//     return derived;
// }

// std::vector<UUID> ModuleGraph::getAnnotations(const UUID& sourceId) const {
//     std::vector<UUID> annotations;
    
//     // Find all modules that annotate this one
//     auto it = reverseAdjacency.find(sourceId);
//     if (it != reverseAdjacency.end()) {
//         for (const auto& link : it->second) {
//             if (link->linkType == ModuleLinkType::ANNOTATES && !link->deleted) {
//                 annotations.push_back(link->sourceId);
//             }
//         }
//     }
    
//     return annotations;
// }
