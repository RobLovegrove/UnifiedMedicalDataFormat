#ifndef READER_HPP
#define READER_HPP

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
#include "umdfFile.hpp"

class Reader {
private:

    Header header;
    XRefTable xrefTable;

    static constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 2 << 20; // 1 << 20 is bitwise 1 * 2^20 = 1,048,576 (1 megabyte) 

    std::ifstream fileStream;
    std::vector<std::unique_ptr<DataModule>> loadedModules;

    bool isOpen() const { return fileStream.is_open(); }

    

public:
    // Reading operations 
    nlohmann::json getFileInfo();
    std::expected<ModuleData, std::string> getModuleData(const std::string& moduleId);
    

    void loadModule(const XrefEntry& entry);

    // File management
    bool openFile(const std::string& filename);
    void closeFile();

};


#endif