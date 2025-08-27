#ifndef WRITER_HPP
#define WRITER_HPP

#include <memory>
#include <string>
#include <fstream>
#include <expected>
#include <vector>

#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include "DataModule/ModuleData.hpp"
#include "Utility/uuid.hpp"
#include "Utility/Encryption/encryptionManager.hpp"
#include "Links/moduleGraph.hpp"
#include "Links/moduleLink.hpp"

struct Result {
    bool success;
    std::string message;
};


class Writer {
private:
    Header header;
    XRefTable xrefTable;
    ModuleGraph moduleGraph;

    std::string filePath;
    std::string tempFilePath;

    std::fstream fileStream;   

    bool newFile = false;

    bool writeXref(std::ostream& outfile);

    Result writeModule(
        std::ostream& outfile, const std::string& schemaPath, UUID moduleId, 
        const ModuleData& moduleData, EncryptionData encryptionData);

    void removeTempFile();
    bool renameTempFile(const std::string& newFilename);
    Result validateTempFile(size_t moduleCount = 0);

    Result setUpFileStream(std::string& filename);
    void resetWriter();

public:

    void printEncounterPath(const UUID& encounterId);

    Result createNewFile(std::string& filename, std::string password = "");
    Result openFile(std::string& filename, std::string password = "");


    Result addModule(const std::string& schemaPath, UUID moduleId, const ModuleData& module);

    Result updateModule(const std::string& moduleId, const ModuleData& module);

    // ModuleGraph methods
    // Start a new encounter. The "root" module defines the encounter.
    std::expected<UUID, std::string> createNewEncounter();
    std::expected<UUID, std::string> addModuleToEncounter(const UUID& encounterId, const std::string& schemaPath, const ModuleData& module);
    std::expected<UUID, std::string> addDerivedModule(const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module);
    std::expected<UUID, std::string> addAnnotation(const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module);

    // // Low level API call available
    // Result addLink(const UUID& sourceId, const UUID& targetId, ModuleLinkType linkType);


    Result closeFile();

};


#endif