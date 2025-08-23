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
#include "Utility/Encryption/EncryptionManager.hpp"

struct Result {
    bool success;
    std::string message;
};


class Writer {
private:
    Header header;
    XRefTable xrefTable;

    std::string filePath;
    std::string tempFilePath;

    std::fstream fileStream;   

    bool newFile = false;

    bool writeXref(std::ostream& outfile);

    std::expected<UUID, std::string> writeModule(
        std::ostream& outfile, const std::string& schemaPath, const ModuleData& moduleData, EncryptionData encryptionData);

    void removeTempFile();
    bool renameTempFile(const std::string& newFilename);
    Result validateTempFile(size_t moduleCount = 0);

    Result setUpFileStream(std::string& filename);
    void resetWriter();

public:
    Result createNewFile(std::string& filename);
    Result openFile(std::string& filename);
    std::expected<UUID, std::string> addModule(const std::string& schemaPath, const ModuleData& module);
    Result updateModule(const std::string& moduleId, const ModuleData& module);
    Result closeFile();


    // Writing operations
    std::expected<std::vector<UUID>, std::string> writeNewFile(std::string& filename, 
        std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas);
    std::expected<std::vector<UUID>, std::string> addModules(
        std::string& filename, std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas);

    bool updateModules(
        std::string& filename,
        std::vector<std::pair<std::string, ModuleData>>& moduleUpdates);

};


#endif