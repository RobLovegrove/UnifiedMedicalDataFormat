#include "EncryptionManager.hpp"
#include <stdexcept>
#include <mutex>
#include <iostream>

// Static member for one-time initialization
static std::once_flag initFlag;

// Private method to ensure libsodium is initialized
static void ensureInitialized() {
    std::call_once(initFlag, []() {
        if (sodium_init() < 0) {
            throw std::runtime_error("Failed to initialize libsodium");
        }
    });
}

bool EncryptionManager::initialize() {
    try {
        ensureInitialized();
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<uint8_t> EncryptionManager::deriveKeyArgon2id(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    uint64_t memoryCost,
    uint32_t timeCost,
    uint32_t /* parallelism */
) {
    ensureInitialized();  // Ensure libsodium is initialized
    

    

    

    if (memoryCost < crypto_pwhash_MEMLIMIT_MIN || 
        memoryCost > crypto_pwhash_MEMLIMIT_MAX) {
        std::cout << "ERROR: Memory cost " << memoryCost << " out of range [" 
             << crypto_pwhash_MEMLIMIT_MIN << ", " 
             << crypto_pwhash_MEMLIMIT_MAX << "]!" << std::endl;
        throw std::runtime_error("Memory cost out of range");
    }
    
    if (timeCost < crypto_pwhash_OPSLIMIT_MIN || 
        timeCost > crypto_pwhash_OPSLIMIT_MAX) {
        std::cout << "ERROR: Time cost " << timeCost << " out of range [" 
             << crypto_pwhash_OPSLIMIT_MIN << ", " 
             << crypto_pwhash_OPSLIMIT_MAX << "]!" << std::endl;
        throw std::runtime_error("Time cost out of range");
    }
    
    std::cout << "All parameters valid, calling crypto_pwhash_argon2id..." << std::endl;
    
    std::vector<uint8_t> key(32); // 256-bit key for AES-256

    
    int result = crypto_pwhash(
        key.data(), key.size(),
        password.c_str(), password.length(),
        salt.data(),
        timeCost, memoryCost, crypto_pwhash_ALG_ARGON2ID13
    );
    
    if (result != 0) {
        throw std::runtime_error("Argon2id key derivation failed");
    }
    
    return key;
}

std::vector<uint8_t> EncryptionManager::generateSalt(size_t length) {
    ensureInitialized();  // Ensure libsodium is initialized
    
    std::vector<uint8_t> salt(length);
    randombytes_buf(salt.data(), length);
    return salt;
}

std::vector<uint8_t> EncryptionManager::generateIV(size_t length) {
    ensureInitialized();  // Ensure libsodium is initialized
    
    std::vector<uint8_t> iv(length);
    randombytes_buf(iv.data(), length);
    return iv;
}

std::vector<uint8_t> EncryptionManager::encryptAES256GCM(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    std::vector<uint8_t>& authTag
) {
    ensureInitialized();  // Ensure libsodium is initialized
    
    if (key.size() != 32) {
        throw std::runtime_error("AES-256-GCM requires 32-byte key");
    }
    if (iv.size() != crypto_aead_aes256gcm_NPUBBYTES) {
        throw std::runtime_error("AES-256-GCM requires 12-byte IV");
    }
    
    std::vector<uint8_t> ciphertext(data.size() + crypto_aead_aes256gcm_ABYTES);
    unsigned long long ciphertextLen;
    
    if (crypto_aead_aes256gcm_encrypt(
        ciphertext.data(), &ciphertextLen,
        data.data(), data.size(),
        nullptr, 0,  // Additional authenticated data
        nullptr,     // Nonce secret (nsec)
        iv.data(),   // Nonce (npub)
        key.data()
    ) != 0) {
        throw std::runtime_error("AES-256-GCM encryption failed");
    }
    
    // Extract auth tag from the end of ciphertext
    authTag.assign(ciphertext.end() - crypto_aead_aes256gcm_ABYTES, ciphertext.end());
    ciphertext.resize(ciphertextLen - crypto_aead_aes256gcm_ABYTES);
    
    return ciphertext;
}

std::vector<uint8_t> EncryptionManager::decryptAES256GCM(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& authTag
) {
    ensureInitialized();  // Ensure libsodium is initialized
    
    if (key.size() != 32) {
        throw std::runtime_error("AES-256-GCM requires 32-byte key");
    }
    if (iv.size() != crypto_aead_aes256gcm_NPUBBYTES) {
        throw std::runtime_error("AES-256-GCM requires 12-byte IV");
    }
    if (authTag.size() != crypto_aead_aes256gcm_ABYTES) {
        throw std::runtime_error("Invalid auth tag size");
    }
    
    // Combine ciphertext and auth tag
    std::vector<uint8_t> combinedData = data;
    combinedData.insert(combinedData.end(), authTag.begin(), authTag.end());
    
    std::vector<uint8_t> plaintext(data.size());
    unsigned long long plaintextLen;
    
    if (crypto_aead_aes256gcm_decrypt(
        plaintext.data(), &plaintextLen,
        nullptr,  // Nonce secret (nsec)
        combinedData.data(), combinedData.size(),
        nullptr, 0,  // Additional authenticated data
        iv.data(),   // Nonce (npub)
        key.data()
    ) != 0) {
        throw std::runtime_error("AES-256-GCM decryption failed");
    }
    
    plaintext.resize(plaintextLen);
    return plaintext;
}

EncryptionType EncryptionManager::decodeEncryptionType(uint8_t value) {
    switch (value) {
        case 1: return EncryptionType::NONE;
        case 2: return EncryptionType::AES_256_GCM;
        default: return EncryptionType::UNKNOWN;
    }
}

std::string EncryptionManager::encryptionToString(EncryptionType encryptionType) {
    switch (encryptionType) {
        case EncryptionType::NONE: return "NONE";
        case EncryptionType::AES_256_GCM: return "AES_256_GCM";
        default: return "UNKNOWN";
    }
}