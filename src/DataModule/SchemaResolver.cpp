#include "SchemaResolver.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

// Initialize static members
std::unordered_map<std::string, nlohmann::json> SchemaResolver::schemaCache;
std::vector<std::string> SchemaResolver::referenceStack;

bool SchemaResolver::hasCircularReference(const std::string& refPath) {
    // Check if this (resolved) reference path is already in the current resolution stack
    return std::find(referenceStack.begin(), referenceStack.end(), refPath) != referenceStack.end();
}

std::string SchemaResolver::resolveRelativePath(const std::string& refPath, const std::string& baseSchemaPath) {
    // If the reference path starts with '/', treat it as absolute from the schemas root
    if (refPath[0] == '/') {
        // Remove the leading '/' and construct the full path
        std::string absolutePath = refPath.substr(1); // Remove leading '/'
        return absolutePath;
    }
    
    // Handle relative paths with proper directory traversal
    fs::path basePath(baseSchemaPath);
    
    if (refPath.size() >= 2 && refPath.substr(0, 2) == "./") {
        // Handle "./path" - relative to current directory
        fs::path baseDir = basePath.parent_path();
        fs::path relativePath(refPath.substr(2)); // Remove "./"
        fs::path fullPath = baseDir / relativePath;
        return fullPath.string();
    } else if (refPath.size() >= 3 && refPath.substr(0, 3) == "../") {
        // Handle "../path" - go up one directory level
        fs::path fullPath = basePath.parent_path().parent_path() / refPath.substr(3); // Remove "../"
        return fullPath.string();
    } else {
        // Handle relative path without "./" - relative to current directory
        fs::path baseDir = basePath.parent_path();
        fs::path fullPath = baseDir / refPath;
        return fullPath.string();
    }
}

nlohmann::json SchemaResolver::getSchemaByResolvedPath(const std::string& fullPath) {
    auto it = schemaCache.find(fullPath);
    if (it != schemaCache.end()) {
        return it->second;
    }

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open referenced schema file: " + fullPath);
    }

    nlohmann::json referencedSchema;
    file >> referencedSchema;
    file.close();

    schemaCache[fullPath] = referencedSchema;
    return referencedSchema;
}

std::string SchemaResolver::beginReference(const std::string& refPath, const std::string& baseSchemaPath) {
    // Resolve full path first and track by resolved path to be robust against differing spellings
    std::string fullPath = resolveRelativePath(refPath, baseSchemaPath);

    if (hasCircularReference(fullPath)) {
        std::string stackTrace = "Circular reference detected: ";
        for (const auto& path : referenceStack) {
            stackTrace += path + " -> ";
        }
        stackTrace += fullPath;
        throw std::runtime_error(stackTrace);
    }

    if (referenceStack.size() >= MAX_REFERENCE_DEPTH) {
        std::string stackTrace = "Schema reference depth exceeded (" + std::to_string(MAX_REFERENCE_DEPTH) + "): ";
        for (const auto& path : referenceStack) {
            stackTrace += path + " -> ";
        }
        stackTrace += fullPath;
        throw std::runtime_error(stackTrace);
    }

    // Push the resolved path for circular detection and return it for IO/cache
    referenceStack.push_back(fullPath);
    return fullPath;
}

void SchemaResolver::endReference() {
    if (!referenceStack.empty()) referenceStack.pop_back();
}

nlohmann::json SchemaResolver::resolveReference(const std::string& refPath, const std::string& baseSchemaPath) {
    // Resolve the full path first to compare canonical paths
    std::string fullPath = resolveRelativePath(refPath, baseSchemaPath);

    // Check for circular references
    if (hasCircularReference(fullPath)) {
        std::string stackTrace = "Circular reference detected: ";
        for (const auto& path : referenceStack) {
            stackTrace += path + " -> ";
        }
        stackTrace += fullPath;
        throw std::runtime_error(stackTrace);
    }
    
    // Check reference depth
    if (referenceStack.size() >= MAX_REFERENCE_DEPTH) {
        std::string stackTrace = "Schema reference depth exceeded (" + 
                                std::to_string(MAX_REFERENCE_DEPTH) + "): ";
        for (const auto& path : referenceStack) {
            stackTrace += path + " -> ";
        }
        stackTrace += fullPath;
        throw std::runtime_error(stackTrace);
    }
    
    // Check if schema is already cached
    if (schemaCache.find(fullPath) != schemaCache.end()) {
        return schemaCache[fullPath];
    }
    
    // Add to current resolution stack using resolved path for circular detection
    referenceStack.push_back(fullPath);
    
    try {
        // Load the referenced schema file
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open referenced schema file: " + fullPath);
        }
        
        nlohmann::json referencedSchema;
        file >> referencedSchema;
        file.close();
        
        // Cache the loaded schema using the resolved path
        schemaCache[fullPath] = referencedSchema;
        
        // Remove from stack before returning
        referenceStack.pop_back();
        
        return referencedSchema;
        
    } catch (const std::exception& e) {
        // Remove from stack before re-throwing
        referenceStack.pop_back();
        throw;
    }
}

void SchemaResolver::clearCache() {
    schemaCache.clear();
    referenceStack.clear();
}

std::vector<std::string> SchemaResolver::getCurrentStack() {
    return referenceStack;
}

bool SchemaResolver::isCached(const std::string& refPath) {
    std::string fullPath = resolveRelativePath(refPath, "");
    return schemaCache.find(fullPath) != schemaCache.end();
}

size_t SchemaResolver::getCacheSize() {
    return schemaCache.size();
}
