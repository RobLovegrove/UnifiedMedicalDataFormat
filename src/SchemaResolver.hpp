#ifndef SCHEMA_RESOLVER_HPP
#define SCHEMA_RESOLVER_HPP

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

class SchemaResolver {
private:
    // Static cache for loaded schemas to avoid reloading
    static std::unordered_map<std::string, nlohmann::json> schemaCache;
    
    // Current resolution stack to detect circular references
    static std::vector<std::string> referenceStack;
    
    // Maximum depth to prevent excessive nesting
    static constexpr int MAX_REFERENCE_DEPTH = 50;
    
    // Helper method to check for circular references
    static bool hasCircularReference(const std::string& refPath);
    
    // Helper method to resolve relative paths
    static std::string resolveRelativePath(const std::string& refPath, const std::string& baseSchemaPath);

public:
    // Main method to resolve schema references (loads from disk and caches)
    static nlohmann::json resolveReference(const std::string& refPath, const std::string& baseSchemaPath);

    // New: begin/end guarded resolution to keep the reference on the stack
    static std::string beginReference(const std::string& refPath, const std::string& baseSchemaPath);
    static void endReference();

    // New: fetch a schema by fully resolved path (cached or load)
    static nlohmann::json getSchemaByResolvedPath(const std::string& fullPath);
    
    // Clear the schema cache (useful for testing or memory management)
    static void clearCache();
    
    // Get the current reference stack (useful for debugging)
    static std::vector<std::string> getCurrentStack();
    
    // Check if a schema is already cached
    static bool isCached(const std::string& refPath);
    
    // Get cache statistics
    static size_t getCacheSize();
};

#endif // SCHEMA_RESOLVER_HPP
