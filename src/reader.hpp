#ifndef READER_HPP
#define READER_HPP

#include <memory>
#include <string>
#include <fstream>
#include <expected>
#include <vector>
#include <optional>
#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include "DataModule/ModuleData.hpp"
#include "Utility/uuid.hpp"
#include "./writer.hpp"
#include "./AuditTrail/auditTrail.hpp"


/**
 * @brief Reader class for reading and accessing UMDF (Unified Medical Data Format) files.
 * 
 * The Reader provides functionality to open, read, and extract data from UMDF files.
 * It supports both encrypted and unencrypted files, and provides access to module data,
 * audit trails, and file metadata. The reader uses a lazy-loading approach where modules
 * are loaded into memory only when requested, with a configurable maximum in-memory size.
 * 
 * Key features:
 * - File encryption support (AES-256-GCM)
 * - Lazy module loading with memory management
 * - Audit trail access for module modification history
 * - Cross-reference table navigation
 * - Module graph traversal for encounter-based data access
 * 
 */
class Reader {
private:
    Header header;
    XRefTable xrefTable;
    ModuleGraph moduleGraph;
    
    static constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 512 * 1024 * 1024; // 500 MB

    std::ifstream fileStream;

    std::vector<std::unique_ptr<DataModule>> loadedModules;
    std::unique_ptr<AuditTrail> auditTrail;

    /**
     * @brief Check if the file stream is currently open.
     * @return true if file is open, false otherwise
     */
    bool isOpen() const { return fileStream.is_open(); }


    /**
     * @brief Load a specific module from the file into memory.
     * 
     * This method reads a module from the specified file offset and size,
     * creates the appropriate DataModule instance based on the module type,
     * and loads it into the module cache.
     * 
     * @param offset File offset where the module is located
     * @param size Size of the module in bytes
     * @param type Type of module to load (determines which DataModule subclass to instantiate)
     * @return std::expected containing the loaded DataModule on success, or error message on failure
     */
    std::expected<std::unique_ptr<DataModule>, std::string> loadModule
        (uint64_t offset, uint32_t size, ModuleType type);

public:

    /**
     * @brief Retrieve module data by module ID.
     * 
     * Loads and returns the complete module data (metadata and content) for the specified module ID.
     * The module is loaded from the file if not already in memory, following the lazy-loading pattern.
     * 
     * @param moduleId String representation of the module UUID
     * @return std::expected containing ModuleData on success, or error message on failure
     * 
     * @note This method will load the module into memory if not already cached
     */
    std::expected<ModuleData, std::string> getModuleData(const std::string& moduleId);
    

     /**
     * @brief Get the complete audit trail for a specific module.
     * 
     * Returns the full history of modifications made to a module, including timestamps,
     * authors and its offset within the file.
     * 
     * @param moduleId UUID of the module to get audit trail for
     * @return std::expected containing vector of ModuleTrail entries on success, or error message on failure
     */
    std::expected<std::vector<ModuleTrail>, std::string> getAuditTrail(const UUID& moduleId);

    /**
     * @brief Get the data content of a specific audit trail entry.
     * 
     * Retrieves the actual module data via offset, stored in the ModuleTrail entry.
     * 
     * @param module The ModuleTrail entry of the data desired
     * @return std::expected containing ModuleData on success, or error message on failure
     */
    std::expected<ModuleData, std::string> getAuditData(const ModuleTrail& module);
    
    // File management
    /**
     * @brief Open a UMDF file for reading.
     * 
     * Opens the specified UMDF file and initializes the reader. The file can be
     * encrypted (requiring a password) or unencrypted. The method reads the file
     * header, cross-reference table, and module graph to prepare for data access.
     * 
     * @param filename Path to the UMDF file to open
     * @param password Optional password for encrypted files (empty string for unencrypted)
     * @return Result indicating success or failure with descriptive message
     * 
     * @note The file remains open until closeFile() is called
     */
    Result openFile(const std::string& filename, std::string password = "");

    /**
     * @brief Close the currently open file and release resources.
     * 
     * Closes the file stream, and cleans up any loaded modules
     * from memory. This should be called when finished reading the file.
     * 
     * @return Result indicating success or failure with descriptive message
     */
    Result closeFile();

    // Reading operations 
    /**
     * @brief Get comprehensive file information and metadata.
     * 
     * Returns a JSON object containing all file metadata, including:
     * - Module graph structure (encounters, modules, relationships)
     * - Cross-reference table summary
     * 
     * This method provides a complete overview of the file contents without
     * loading individual module data into memory.
     * 
     * @return nlohmann::json object containing structured file information
     * 
     * @note This is a lightweight operation that doesn't load module data
     * @note The returned JSON can be used for file browsing and navigation
     */
    nlohmann::json getFileInfo();
};
#endif