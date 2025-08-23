#ifndef ENCRYPTIONMANAGER_HPP
#define ENCRYPTIONMANAGER_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <sodium.h>

enum class EncryptionType {
    UNKNOWN = 0,
    NONE = 1,
    AES_256_GCM = 2
};

struct EncryptionData {
    EncryptionType encryptionType = EncryptionType::NONE;

    std::string masterPassword = "password";
    
    // For Argon2id key derivation:
    std::vector<uint8_t> baseSalt;        // Master salt for Argon2id
    uint64_t memoryCost;                  // Memory cost in bytes (e.g., 65536 = 64KB)
    uint32_t timeCost;                    // Time cost/iterations (e.g., 3)
    uint32_t parallelism;                 // Parallelism (usually 1)
    
    // AES256-GCM parameters:
    std::vector<uint8_t> moduleSalt;      // Module-specific salt
    std::vector<uint8_t> iv;             // Nonce (12 bytes for GCM)
    std::vector<uint8_t> authTag{crypto_aead_aes256gcm_ABYTES};        // Authentication tag (16 bytes)
};


class EncryptionManager {
public:
    // Key derivation using Argon2id
    static std::vector<uint8_t> deriveKeyArgon2id(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        uint64_t memoryCost,
        uint32_t timeCost,
        uint32_t parallelism
    );
    
    // Salt and IV generation
    static std::vector<uint8_t> generateSalt(size_t length = 32);
    static std::vector<uint8_t> generateIV(size_t length = 12);
    
    // Main encryption/decryption
    static std::vector<uint8_t> encryptAES256GCM(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        std::vector<uint8_t>& authTag
    );
    
    static std::vector<uint8_t> decryptAES256GCM(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& authTag
    );
    
    // Utility methods
    static EncryptionType decodeEncryptionType(uint8_t value);
    static std::string encryptionToString(EncryptionType encryptionType);

    static void ensureInitialized();
};





#endif