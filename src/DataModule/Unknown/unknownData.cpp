#include "unknownData.hpp"
#include "../Image/imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Xref/xref.hpp"
#include "../stringBuffer.hpp"
#include "../Header/dataHeader.hpp"
#include "../../Utility/Compression/ZstdCompressor.hpp"
#include "../../Utility/Encryption/encryptionManager.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

UnknownData::UnknownData(const string& schemaPath, DataHeader& dataheader) : DataModule(schemaPath, dataheader) {
    initialise();
}

UnknownData::UnknownData(
    const string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, EncryptionData encryptionData) 
    : DataModule(schemaPath, schemaJson, uuid, ModuleType::Unknown, encryptionData) {
    initialise();
}

// Override the virtual method for tabular-specific data
std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    UnknownData::getModuleSpecificData() const {

        // Return JSON with error that data is unknown
        nlohmann::json json;
        json["error"] = "Data type is unknown and therefore cannot be read";
        return json;
} 