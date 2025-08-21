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

class Writer {
private:
    Header header;
    XRefTable xrefTable;
    std::string tempFilePath;

    bool writeXref(std::ostream& outfile);

    std::expected<UUID, std::string> writeModule(
        std::ostream& outfile, const std::string& schemaPath, const ModuleData& moduleData, EncryptionData encryptionData);

    void cleanupTempFile();
    bool renameTempFile(const std::string& newFilename);
    bool validateTempFile(size_t moduleCount = 0);

public:

    // Writing operations (delegated to Writer)
    std::expected<std::vector<UUID>, std::string> writeNewFile(std::string& filename, 
        std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas);
    std::expected<std::vector<UUID>, std::string> addModules(
        std::string& filename, std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas);

    bool updateModules(
        std::string& filename,
        std::vector<std::pair<std::string, ModuleData>>& moduleUpdates);

};


#endif