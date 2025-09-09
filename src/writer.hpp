#ifndef WRITER_HPP
#define WRITER_HPP

#include <memory>
#include <string>
#include <fstream>
#include <expected>
#include <vector>
#include <boost/interprocess/sync/file_lock.hpp>

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

    std::unique_ptr<boost::interprocess::file_lock> fileLock;

    std::string filePath;
    std::string tempFilePath;

    std::fstream fileStream;   

    std::string author;
    bool newFile = false;

    void releaseFileLock();

    bool writeXref(std::ostream& outfile);

    Result writeModule(
        std::ostream& outfile, const std::string& schemaPath, UUID moduleId, 
        const ModuleData& moduleData, EncryptionData encryptionData);

    void removeTempFile();
    bool renameTempFile(const std::string& newFilename);
    Result validateTempFile(size_t moduleCount = 0);

    Result setUpFileStream(std::string& filename);
    void resetWriter();

    Result addModule(const std::string& schemaPath, UUID moduleId, const ModuleData& module);

public:

    ~Writer();

    void printEncounterPath(const UUID& encounterId);

    Result createNewFile(std::string& filename, std::string author, std::string password = "");
    Result openFile(std::string& filename, std::string author, std::string password = "");

    Result updateModule(const std::string& moduleId, const ModuleData& module);

    // ModuleGrph methods
    // Start a new encounter. The "root" module defines the encounter.
    std::expected<UUID, std::string> createNewEncounter();
    std::expected<UUID, std::string> addModuleToEncounter(const UUID& encounterId, const std::string& schemaPath, const ModuleData& module);
    std::expected<UUID, std::string> addVariantModule(const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module);
    std::expected<UUID, std::string> addAnnotation(const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module);
    
    Result cancelThenClose();
    Result closeFile();

};


#endif