#include "moduleGraph.hpp"
#include "../Utility/tlvHeader.hpp"
#include "../Utility/moduleType.hpp"

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <stack>
#include <sstream>
#include <functional>

using namespace std;

/* READER FUNCTIONS */

ModuleGraph ModuleGraph::readModuleGraph(std::istream& in) {

    ModuleGraph moduleGraph;

    // Read graph header
    try {
        moduleGraph.readGraphHeader(in);
    } catch (const std::exception& e) {
        throw std::runtime_error("Exception reading graph header: " + std::string(e.what()));
    }

    // Read encounters
    try {
        moduleGraph.readEncounters(in);
    } catch (const std::exception& e) {
        throw std::runtime_error("Exception reading encounters: " + std::string(e.what()));
    }

    // Read links
    try {
        moduleGraph.readLinks(in);
    } catch (const std::exception& e) {
        throw std::runtime_error("Exception reading links: " + std::string(e.what()));
    }

    // // Build adjacency lists
    // moduleGraph.buildAdjacencyLists();

    // if (moduleGraph.hasCycle()) {
    //     throw std::runtime_error("Cycle detected when building module graph");
    // }

    return moduleGraph;
}

void ModuleGraph::readGraphHeader(std::istream& in) {

    // Read header size
    uint8_t typeId;
    uint32_t length;
    uint32_t headerSize;
    
    in.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
    in.read(reinterpret_cast<char*>(&length), sizeof(length));

    if (typeId != static_cast<uint8_t>(HeaderFieldType::HeaderSize)) {
        throw std::runtime_error("Invalid header: expected HeaderSize first.");
    }

    in.read(reinterpret_cast<char*>(&headerSize), sizeof(headerSize));

    size_t bytesRead = sizeof(typeId) + sizeof(length) + sizeof(headerSize);    
    while (bytesRead < headerSize) {
        in.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
        in.read(reinterpret_cast<char*>(&length), sizeof(length));
        bytesRead += sizeof(typeId) + sizeof(length);

        std::vector<char> buffer(length);
        in.read(buffer.data(), length);
        bytesRead += length;

        auto type = static_cast<HeaderFieldType>(typeId);
        switch (type) {
            case HeaderFieldType::EncounterSize:
                if (length != sizeof(encounterSize)) throw std::runtime_error("Invalid EncounterSize length.");
                std::memcpy(&encounterSize, buffer.data(), sizeof(encounterSize));
                break;
            case HeaderFieldType::LinkSize:
                if (length != sizeof(linkSize)) throw std::runtime_error("Invalid LinkSize length.");
                std::memcpy(&linkSize, buffer.data(), sizeof(linkSize));
                break;
            default:
                throw std::runtime_error("Invalid ModuleGraph header field type: " + std::to_string(typeId));
        }
    }
}

void ModuleGraph::readEncounters(std::istream& in) {

    size_t bytesRead = 0;
    while (bytesRead < encounterSize) {
        cout << "Reading encounter" << endl;
        Encounter encounter;

        std::array<uint8_t, 16> temp;
        UUID tempUUID;

        in.read(reinterpret_cast<char*>(temp.data()), temp.size());
        tempUUID.setData(temp);
        encounter.encounterId = tempUUID;

        in.read(reinterpret_cast<char*>(temp.data()), temp.size());
        tempUUID.setData(temp);
        encounter.rootModule = tempUUID;

        in.read(reinterpret_cast<char*>(temp.data()), temp.size());
        tempUUID.setData(temp);
        encounter.lastModule = tempUUID;

        bytesRead += temp.size() * 3;

        encounters[encounter.encounterId] = encounter;
    }

    cout << "Encounters read" << endl;

    for (const auto& [encounterId, encounter] : encounters) {
        cout << "Encounter " << encounterId.toString() << ": " << encounter.rootModule.value().toString() << " -> " << encounter.lastModule.value().toString() << endl;
    }
}

void ModuleGraph::readLinks(std::istream& in) {
    size_t bytesRead = 0;
    while (bytesRead < linkSize) {
        UUID sourceId;
        UUID targetId;
        ModuleLinkType linkType;
        bool deleted;

        std::array<uint8_t, 16> temp;
        
        in.read(reinterpret_cast<char*>(temp.data()), temp.size());
        sourceId.setData(temp);

        in.read(reinterpret_cast<char*>(temp.data()), temp.size());
        targetId.setData(temp);

        in.read(reinterpret_cast<char*>(&linkType), sizeof(linkType));
        in.read(reinterpret_cast<char*>(&deleted), sizeof(deleted));
        bytesRead += temp.size() * 2 + sizeof(linkType) + sizeof(deleted);

        // Skip deleted links early
        if (deleted) {
            continue;
        }

        // Before inserting, check for cycles
        if (wouldCreateCycle(sourceId, targetId)) {
            throw std::runtime_error(
                "Cycle detected while reading links: " +
                sourceId.toString() + " -> " + targetId.toString()
            );
        }

        // Create and store link
        auto linkPtr = std::make_shared<ModuleLink>(sourceId, targetId, linkType);
        linkPtr->deleted = deleted;
        links.push_back(linkPtr);

        // Update adjacency maps immediately
        adjacency[sourceId].push_back(linkPtr);
        reverseAdjacency[targetId].push_back(linkPtr);
    }
}

// void ModuleGraph::buildAdjacencyLists() {
//     adjacency.clear();
//     reverseAdjacency.clear();

//     for (const auto& linkPtr : links) {
//         if (linkPtr->deleted) continue; // skip deleted links

//         // Forward adjacency: source -> target(s)
//         adjacency[linkPtr->sourceId].push_back(linkPtr);

//         // Reverse adjacency: target -> source(s)
//         reverseAdjacency[linkPtr->targetId].push_back(linkPtr);
//     }
// }

// bool ModuleGraph::hasCycle() const {
//     std::unordered_set<UUID> visited;
//     std::unordered_set<UUID> recursionStack;

//     std::function<bool(const UUID&)> dfs = [&](const UUID& node) -> bool {
//         if (recursionStack.count(node)) return true; // back edge → cycle
//         if (visited.count(node)) return false;

//         visited.insert(node);
//         recursionStack.insert(node);

//         auto it = adjacency.find(node);
//         if (it != adjacency.end()) {
//             for (const auto& link : it->second) {
//                 if (dfs(link->targetId)) {
//                     return true;
//                 }
//             }
//         }

//         recursionStack.erase(node);
//         return false;
//     };

//     // Run DFS from every node
//     for (const auto& [node, _] : adjacency) {
//         if (dfs(node)) return true;
//     }
//     return false;
// }


// Writing methods

uint32_t ModuleGraph::writeModuleGraph(std::ostream& outfile) {

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

    // Update the header size
    updateHeader(buffer);

    outfile << buffer.str();

    return buffer.str().size();
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

    encounterSize = 0;
    for (auto& [encounterId, encounter] : encounters) {

        if (encounter.rootModule.has_value()) {

            outfile.write(reinterpret_cast<const char*>(encounterId.data().data()), encounterId.data().size());
            outfile.write(reinterpret_cast<const char*>(encounter.rootModule.value().data().data()), encounter.rootModule.value().data().size());
            encounterSize += encounterId.data().size() + encounter.rootModule.value().data().size();

            if (encounter.lastModule.has_value()) {
                outfile.write(reinterpret_cast<const char*>(encounter.lastModule.value().data().data()), encounter.lastModule.value().data().size());
                encounterSize += encounter.lastModule.value().data().size();
            }
            else {
                outfile.write(reinterpret_cast<const char*>(encounter.rootModule.value().data().data()), encounter.rootModule.value().data().size());
                encounterSize += encounter.rootModule.value().data().size();
            }
        }
    }
}

void ModuleGraph::writeLinks(std::ostream& outfile) {

    cout << "Writing links" << endl;
    linkSize = 0;
    for (const std::shared_ptr<ModuleLink>& link : links) {

        outfile.write(reinterpret_cast<const char*>(link->sourceId.data().data()), link->sourceId.data().size());
        outfile.write(reinterpret_cast<const char*>(link->targetId.data().data()), link->targetId.data().size());
        outfile.write(reinterpret_cast<const char*>(&link->linkType), sizeof(link->linkType));
        outfile.write(reinterpret_cast<const char*>(&link->deleted), sizeof(link->deleted));

        linkSize += link->sourceId.data().size() + link->targetId.data().size() + sizeof(link->linkType) + sizeof(link->deleted);
    }
}

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

    Encounter& encounter = getEncounter(encounterId);
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


void ModuleGraph::addModuleLink(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType) {

    ModuleLink link(parentModuleId, moduleId, linkType);
    addLink(link);
}

void ModuleGraph::removeModuleLink(const UUID& parentModuleId, const UUID& moduleId, ModuleLinkType linkType) {

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

void ModuleGraph::displayEncounters() const {
    for (const auto& [encounterId, encounter] : encounters) {
        if (!encounter.rootModule.has_value()) {
            std::cout << "Encounter " << encounterId.toString() << " (no root)\n";
            continue;
        }

        std::cout << "Encounter " << encounterId.toString() << ": ";

        // Start with root
        UUID current = encounter.rootModule.value();
        std::cout << current.toString();

        // Follow BelongsTo chain until lastModule
        while (current != encounter.lastModule) {
        auto it = adjacency.find(current);
            if (it == adjacency.end()) break; // no outgoing links

            // Look for the BelongsTo chain link
            UUID next{};
            bool found = false;
            for (const auto& link : it->second) {
                if (link->linkType == ModuleLinkType::BELONGS_TO) {
                    next = link->targetId;
                    found = true;
                    break;
                }
            }

            if (!found) break; // broken chain

            std::cout << " -> " << next.toString();
            current = next;
        }

        std::cout << "\n";
    }
}

void ModuleGraph::displayLinks() const {
    std::cout << "==== ModuleGraph Links ====" << std::endl;

    if (links.empty()) {
        std::cout << "No links present." << std::endl;
        return;
    }

    for (const auto& linkPtr : links) {
        std::cout 
            << "Source: " << linkPtr->sourceId.toString()
            << " -> Target: " << linkPtr->targetId.toString()
            << " | Type: " << static_cast<int>(linkPtr->linkType) // or create a helper to convert to string
            << " | Deleted: " << (linkPtr->deleted ? "Yes" : "No")
            << std::endl;
    }

    std::cout << "============================" << std::endl;
}


void ModuleGraph::printEncounterPath(const UUID& encounterId) const {
    auto encounterIt = encounters.find(encounterId);
    if (encounterIt == encounters.end()) {
        std::cout << "Encounter " << encounterId.toString() << " not found.\n";
        return;
    }

    const Encounter& encounter = encounterIt->second;

    if (!encounter.rootModule.has_value()) {
        std::cout << "Encounter " << encounterId.toString() << " has no root module.\n";
        return;
    }

    std::cout << "Printing Encounter:\n";
    std::cout << "Encounter ID: " << encounterId.toString() << "\n\n";

    // Recursive helper to print a module and its variant/annotated children
    std::function<void(const UUID&, int)> printModule;
    printModule = [&](const UUID& moduleId, int indent) {
        // Print module ID with indentation
        for (int i = 0; i < indent; ++i) std::cout << "    ";
        std::cout << moduleId.toString() << "\n";

        // Print variant modules (indented)
        auto variantIt = adjacency.find(moduleId);
        if (variantIt != adjacency.end()) {
            for (const auto& link : variantIt->second) {
                if (link->linkType == ModuleLinkType::VARIANT_OF) {
                    printModule(link->targetId, indent + 1);
                }
            }
        }

        // Print annotations (with arrow)
        if (variantIt != adjacency.end()) {
            for (const auto& link : variantIt->second) {
                if (link->linkType == ModuleLinkType::ANNOTATES) {
                    for (int i = 0; i < indent; ++i) std::cout << "    ";
                    std::cout << "-> " << link->targetId.toString() << "\n";
                }
            }
        }
    };

    // Traverse BELONGS_TO chain along encounter
    UUID current = encounter.rootModule.value();
    while (true) {
        printModule(current, 0);

        if (current == encounter.lastModule) break;

        // Move to next module in BELONGS_TO chain
        auto it = adjacency.find(current);
        if (it == adjacency.end()) break;

        bool foundNext = false;
        for (const auto& link : it->second) {
            if (link->linkType == ModuleLinkType::BELONGS_TO) {
                current = link->targetId;
                foundNext = true;
                break;
            }
        }
        if (!foundNext) break;
    }
}

// JSON export methods
nlohmann::json ModuleGraph::toJson() const {
    std::cout << "DEBUG: ModuleGraph::toJson() called!" << std::endl;
    
    // Debug: Print all module links
    std::cout << "DEBUG: All Module Links:" << std::endl;
    for (const auto& link : links) {
        if (!link->deleted) {
            std::cout << "  " << link->sourceId.toString() << " -> " << link->targetId.toString() 
                      << " (" << static_cast<int>(link->linkType) << ")" << std::endl;
        }
    }
    
    // Debug: Print adjacency lists
    std::cout << "DEBUG: Adjacency (outgoing links):" << std::endl;
    for (const auto& [moduleId, links] : adjacency) {
        std::cout << "  " << moduleId.toString() << " -> ";
        for (const auto& link : links) {
            if (!link->deleted) {
                std::cout << link->targetId.toString() << "(" << static_cast<int>(link->linkType) << ") ";
            }
        }
        std::cout << std::endl;
    }
    
    // Debug: Print reverse adjacency lists
    std::cout << "DEBUG: Reverse Adjacency (incoming links):" << std::endl;
    for (const auto& [moduleId, links] : reverseAdjacency) {
        std::cout << "  " << moduleId.toString() << " <- ";
        for (const auto& link : links) {
            if (!link->deleted) {
                std::cout << link->sourceId.toString() << "(" << static_cast<int>(link->linkType) << ") ";
            }
        }
        std::cout << std::endl;
    }
    
    nlohmann::json result;
    
    // Build encounters array using the recursive tree approach
    nlohmann::json encountersArray = nlohmann::json::array();
    for (const auto& [encounterId, encounter] : encounters) {
        // Use the new recursive tree approach for each encounter
        nlohmann::json encounterTree = encounterToJson(encounterId);
        if (!encounterTree.empty()) {
            encountersArray.push_back(encounterTree);
        }
    }
    
    result["encounters"] = encountersArray;
    
    // Add graph summary
    result["module_graph"] = buildGraphSummary();
    
    return result;
}

// Recursive helper: build JSON for one module and its descendants
nlohmann::json ModuleGraph::moduleToJson(const UUID& moduleId) const {
    std::cout << "DEBUG: moduleToJson called for module: " << moduleId.toString() << std::endl;
    
    nlohmann::json j;
    j["id"] = moduleId.toString();

    nlohmann::json variantArray = nlohmann::json::array();
    nlohmann::json annotatesArray = nlohmann::json::array();

    // Find annotations (incoming links)
    auto it = reverseAdjacency.find(moduleId);
    if (it != reverseAdjacency.end()) {
        for (const auto& link : it->second) {
            if (link->deleted) continue;

            if (link->linkType == ModuleLinkType::ANNOTATES) {
                annotatesArray.push_back(moduleToJson(link->sourceId));
            }
        }
    }

    // Find variant modules (incoming links)
    std::cout << "DEBUG: Looking for VARIANT_OF links for module: " << moduleId.toString() << std::endl;
    
    // Debug: Check if there are any circular references
    if (reverseAdjacency.find(moduleId) != reverseAdjacency.end()) {
        std::cout << "DEBUG: reverseAdjacency contains module: " << moduleId.toString() << std::endl;
        const auto& links = reverseAdjacency.at(moduleId);
        std::cout << "DEBUG: Number of incoming links: " << links.size() << std::endl;
        
        for (size_t i = 0; i < links.size(); i++) {
            const auto& link = links[i];
            std::cout << "DEBUG: Link " << i << ": " << link->sourceId.toString() << " -> " << link->targetId.toString() 
                      << " (type: " << static_cast<int>(link->linkType) << ", deleted: " << link->deleted << ")" << std::endl;
            
            // Check for circular reference
            if (link->sourceId == moduleId) {
                std::cout << "DEBUG: WARNING - Circular reference detected! Link points to itself!" << std::endl;
            }
        }
    } else {
        std::cout << "DEBUG: No incoming links found for module: " << moduleId.toString() << std::endl;
    }
    
    auto reverseIt = reverseAdjacency.find(moduleId);
    if (reverseIt != reverseAdjacency.end()) {
        for (const auto& link : reverseIt->second) {
            if (link->deleted) continue;

            if (link->linkType == ModuleLinkType::VARIANT_OF) {
                std::cout << "DEBUG: Found VARIANT_OF link! Adding to variantArray" << std::endl;
                variantArray.push_back(moduleToJson(link->sourceId));
            }
        }
    }

    if (!variantArray.empty())
        j["variant"] = variantArray;
    if (!annotatesArray.empty())
        j["annotated_by"] = annotatesArray;

    return j;
}

// Public entry: build full tree for an encounter
nlohmann::json ModuleGraph::encounterToJson(const UUID& encounterId) const {
    auto it = encounters.find(encounterId);
    if (it == encounters.end() || !it->second.rootModule.has_value()) {
        return {}; // no encounter or no root
    }

    nlohmann::json rootJson;
    rootJson["encounter_id"] = encounterId.toString();
    
    // Build the complete module tree: linear chain + variant modules + annotations
    nlohmann::json moduleTree = nlohmann::json::array();
    
    // Start with root module and follow BELONGS_TO chain
    UUID current = it->second.rootModule.value();
    std::unordered_set<UUID> visited;
    
    while (true) {
        // Add current module with its variant data and annotations
        nlohmann::json moduleJson = moduleToJson(current);
        moduleTree.push_back(moduleJson);
        visited.insert(current);
        
        // Check if we've reached the last module
        if (it->second.lastModule.has_value() && current == it->second.lastModule.value()) {
            break;
        }
        
        // Find next module in BELONGS_TO chain
        auto adjIt = adjacency.find(current);
        if (adjIt == adjacency.end()) break;
        
        bool foundNext = false;
        for (const auto& link : adjIt->second) {
            if (link->linkType == ModuleLinkType::BELONGS_TO && !link->deleted) {
                current = link->targetId;
                foundNext = true;
                break;
            }
        }
        
        if (!foundNext || visited.count(current)) break; // Prevent cycles
    }
    
    rootJson["module_tree"] = moduleTree;
    return rootJson;
}


nlohmann::json ModuleGraph::buildGraphSummary() const {
    nlohmann::json summary;
    
    // Count link types
    std::unordered_map<ModuleLinkType, int> linkTypeCounts;
    for (const auto& link : links) {
        if (!link->deleted) {
            linkTypeCounts[link->linkType]++;
        }
    }
    
    summary["total_links"] = links.size();
    summary["active_links"] = std::count_if(links.begin(), links.end(), 
        [](const std::shared_ptr<ModuleLink>& link) { return !link->deleted; });
    
    // Convert enum counts to readable strings
    nlohmann::json linkTypeSummary = nlohmann::json::object();
    for (const auto& [linkType, count] : linkTypeCounts) {
        std::string typeName;
        switch (linkType) {
            case ModuleLinkType::BELONGS_TO: typeName = "belongs_to"; break;
            case ModuleLinkType::VARIANT_OF: typeName = "variant_of"; break;
            case ModuleLinkType::ANNOTATES: typeName = "annotates"; break;
        }
        linkTypeSummary[typeName] = count;
    }
    summary["link_types"] = linkTypeSummary;
    
    summary["total_encounters"] = encounters.size();
    
    return summary;
}