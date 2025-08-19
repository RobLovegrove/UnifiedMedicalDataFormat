#ifndef UMDF_FILE_HPP
#define UMDF_FILE_HPP

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

class UMDFFile {
private:
    std::fstream fileStream;
    Header header;
    XRefTable xrefTable;
    std::vector<std::unique_ptr<DataModule>> loadedModules;

    std::string tempFilePath;

    static constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 2 << 20; // 1 << 20 is bitwise 1 * 2^20 = 1,048,576 (1 megabyte) 

    bool writeXref(std::ostream& outfile);

    std::expected<UUID, std::string> writeModule(
        std::ostream& outfile, const std::string& schemaPath, const ModuleData& moduleData);

    void cleanupTempFile();
    bool renameTempFile(const std::string& newFilename);
    bool validateTempFile(size_t moduleCount = 0);

public:
    UMDFFile() = default;
    ~UMDFFile() = default;
    
    // File management
    bool openFile(const std::string& filename);
    void closeFile();
    bool isOpen() const { return fileStream.is_open(); }
    
    // Reading operations 
    nlohmann::json getFileInfo();
    std::expected<ModuleData, std::string> getModuleData(const std::string& moduleId);

    void loadModule(const XrefEntry& entry);
    
    // Writing operations (delegated to Writer)
    std::expected<std::vector<UUID>, std::string> writeNewFile(std::string& filename, 
        std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas);
    std::expected<std::vector<UUID>, std::string> addModules(
        std::string& filename, std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas);

    bool updateModules(
        std::string& filename,
        std::vector<std::pair<std::string, ModuleData>>& moduleUpdates);
    
    //std::expected<std::vector<UUID>, std::string> deleteModules(const std::string& moduleId);

};

#endif 
