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

/**
 * @brief Result structure for operation status reporting.
 * 
 * Simple structure used throughout the Writer and Reader classes to return method results.
 * Provides a consistent interface for success/failure reporting with descriptive messages.
 */
struct Result {
    bool success;
    std::string message;
};

/**
 * @brief Writer class for creating and modifying UMDF (Unified Medical Data Format) files.
 * 
 * The Writer provides comprehensive functionality for creating new UMDF files and modifying
 * existing ones. It supports both encrypted and unencrypted files, and maintains data integrity
 * through atomic operations.
 * 
 * Key features:
 * - File encryption support (AES-256-GCM)
 * - Atomic write operations
 * - Module graph management for encounter-based data organization
 * - File locking to prevent concurrent access conflicts
 * - Support for variants and annotations linked to parent modules
 * 
 * @note All write operations are atomic - either the entire operation succeeds or fails
 * @note The writer uses temporary files during operations to ensure data integrity
 */
class Writer {
private:

    // Attributes
    Header header;
    XRefTable xrefTable;
    ModuleGraph moduleGraph;

    std::string author;
    bool newFile = false;
    std::unique_ptr<boost::interprocess::file_lock> fileLock;

    // File paths
    std::string filePath;
    std::string tempFilePath;
    std::fstream fileStream;

    /**
     * @brief Release the file lock and clean up lock resources.
     * 
     * Called during cleanup to ensure file locks are properly released,
     * preventing file access issues on subsequent operations.
     */
    void releaseFileLock();

    /**
     * @brief Write the cross-reference table to the output stream.
     * 
     * Writes the complete XRefTable to the specified output stream.
     * 
     * @param outfile Output stream to write the XRefTable to
     * @return true if successful, false otherwise
     */
    bool writeXref(std::ostream& outfile);

    /**
     * @brief Write a single module to the output stream.
     * 
     * Writes a complete module (including metadata, data, and encryption) to the
     * specified output stream.
     * 
     * @param outfile Output stream to write the module to
     * @param schemaPath Path to the JSON schema file for this module
     * @param moduleId UUID of the module being written
     * @param moduleData Complete module data (metadata and content)
     * @param encryptionData Encryption parameters and keys
     * @return Result indicating success or failure with descriptive message
     */
    Result writeModule(
        std::ostream& outfile, const std::string& schemaPath, UUID moduleId, 
        const ModuleData& moduleData, EncryptionData encryptionData);

    /**
     * @brief Remove the temporary file.
     * 
     * Cleans up the temporary file that was created during write operations.
     * This is called after successful operations or during error cleanup.
     */
    void removeTempFile();

    /**
     * @brief Rename the temporary file to the final filename.
     * 
     * Atomically renames the temporary file to the target filename,
     * completing the write operation. This ensures atomicity of the entire operation.
     * 
     * @param newFilename Target filename for the completed file
     * @return true if successful, false otherwise
     */
    bool renameTempFile(const std::string& newFilename);

    /**
     * @brief Validate the temporary file before finalizing the operation.
     * 
     * Performs integrity checks on the temporary file to ensure it's valid
     * before committing the changes. This includes checking file structure,
     * module count, and reading module headers.
     * 
     * @param moduleCount Expected number of modules in the file (0 if check not required)
     * @return Result indicating validation success or failure with details
     */
    Result validateTempFile(size_t moduleCount = 0);

     /**
     * @brief Set up the file stream for the specified filename.
     * 
     * Initializes the file stream and prepares it for writing operations.
     * Handles file creation, locking, and initial setup.
     * 
     * @param filename Path to the file to open for writing
     * @return Result indicating success or failure with descriptive message
     */   
    Result setUpFileStream(std::string& filename);

    /**
     * @brief Reset the writer state to initial conditions.
     * 
     * Clears all internal state and prepares the writer for a new operation.
     * Called when starting new operations or after cleanup.
     */
    void resetWriter();

    /**
     * @brief Add a module to the internal data structures.
     * 
     * Internal method that adds a module to the writer's internal data structures
     * (XRefTable, ModuleGraph) without writing to disk. This is used by
     * public methods to prepare modules for writing.
     * 
     * @param schemaPath Path to the JSON schema file for this module
     * @param moduleId UUID of the module to add
     * @param module Complete module data to add
     * @return Result indicating success or failure with descriptive message
     */
    Result addModule(const std::string& schemaPath, UUID moduleId, const ModuleData& module);

public:

    /**
     * @brief Destructor - ensures proper cleanup of resources.
     * 
     * Releases file locks when the Writer object is destroyed.
     */
    ~Writer();

    /**
     * @brief Create a new UMDF file.
     * 
     * Creates a new UMDF file with the specified filename, author, and optional password.
     * The file is initialized with a header, empty cross-reference table, and module graph.
     * 
     * @param filename Path where the new file should be created
     * @param author Author name for audit trail entries
     * @param password Optional password for file encryption (empty string for unencrypted)
     * @return Result indicating success or failure with descriptive message
     * 
     * @note The file is created but not closed - call closeFile() when finished
     * @note If the file already exists the operation will fail
     * @note File locks are acquired to prevent concurrent access
     */
    Result createNewFile(std::string& filename, std::string author, std::string password = "");

    /**
     * @brief Open an existing UMDF file for modification.
     * 
     * Opens an existing UMDF file for writing. The file can be
     * encrypted (requiring a password) or unencrypted. All existing data is
     * loaded and ready for modification.
     * 
     * @param filename Path to the existing UMDF file
     * @param author Author name for audit trail entries
     * @param password Optional password for encrypted files (empty string for unencrypted)
     * @return Result indicating success or failure with descriptive message
     * 
     * @note The file remains open until closeFile() is called
     * @note File locks are acquired to prevent concurrent access
     */
    Result openFile(std::string& filename, std::string author, std::string password = "");

    /**
     * @brief Update an existing module with new data.
     * 
     * Updates the data content of an existing module identified by its module ID.
     * The module's relationships remain unchanged, but the clinical metadata/data
     * content is replaced with the new data provided.
     * 
     * @param moduleId String representation of the module UUID to update
     * @param module New module data to replace the existing data
     * @return Result indicating success or failure with descriptive message
     * 
     * @note The module must exist in the file for this operation to succeed
     */
    Result updateModule(const std::string& moduleId, const ModuleData& module);

    // ModuleGrph methods
    /**
     * @brief Create a new encounter in the module graph.
     * 
     * Creates a new encounter (patient visit/session) in the module graph.
     * Encounters serve as containers for related modules and provide
     * organizational structure for medical data.
     * 
     * @return std::expected containing the new encounter UUID on success, or error message on failure
     * 
     * @note Encounters are the top-level organizational unit in UMDF files
     * @note Each encounter can contain multiple modules
     */
    std::expected<UUID, std::string> createNewEncounter();

    /**
     * @brief Add a module to an existing encounter.
     * 
     * Adds a new module to the specified encounter. The module is created
     * with the provided schema and data, and linked to the encounter in
     * the module graph.
     * 
     * @param encounterId UUID of the encounter to add the module to
     * @param schemaPath Path to the JSON schema file for this module
     * @param module Complete module data to add
     * @return std::expected containing the new module UUID on success, or error message on failure
     * 
     * @note The encounter must exist for this operation to succeed
     */
    std::expected<UUID, std::string> addModuleToEncounter(const UUID& encounterId, const std::string& schemaPath, const ModuleData& module);

    /**
     * @brief Add a variant module linked to a parent module.
     * 
     * Creates a variant module that is linked to an existing parent module.
     * Variants are used to represent alternative versions or interpretations
     * of the same data (e.g., different analysis results).
     * 
     * @param parentModuleId UUID of the parent module to create a variant for
     * @param schemaPath Path to the JSON schema file for this variant module
     * @param module Complete variant module data
     * @return std::expected containing the new variant module UUID on success, or error message on failure
     * 
     * @note The parent module must exist for this operation to succeed
     * @note Variants maintain a relationship link to their parent module
     */
    std::expected<UUID, std::string> addVariantModule(const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module);

    /**
     * @brief Add an annotation module linked to a parent module.
     * 
     * Creates an annotation module that is linked to an existing parent module.
     * Annotations are used to add supplementary information, comments, or
     * additional context to existing modules (e.g., radiologist notes, segmentation masks).
     * 
     * @param parentModuleId UUID of the parent module to annotate
     * @param schemaPath Path to the JSON schema file for this annotation module
     * @param module Complete annotation module data
     * @return std::expected containing the new annotation module UUID on success, or error message on failure
     * 
     * @note The parent module must exist for this operation to succeed
     * @note Annotations maintain a relationship link to their parent module
     */
    std::expected<UUID, std::string> addAnnotation(const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module);
    
    /**
     * @brief Cancel all pending changes and close the file.
     * 
     * Discards all pending changes and closes the file without saving.
     * This is useful for aborting operations or cleaning up after errors.
     * 
     * @return Result indicating success or failure with descriptive message
     * 
     * @note All changes made since the last closeFile() are lost
     * @note Temporary files are cleaned up
     * @note File locks are released
     */
    Result cancelThenClose();

    /**
     * @brief Close the file and save all changes.
     * 
     * This method writes module graph and cross-reference table to the file.
     * It then validates the file. On successful validation it atomically 
     * renames the temporary file to the final filename.
     * 
     * @return Result indicating success or failure with descriptive message
     * 
     * @note File locks are released after successful close
     */
    Result closeFile();

};


#endif