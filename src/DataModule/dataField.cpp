#include "dataField.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <nlohmann/json.hpp> 

using namespace std;

/* ================== DataField ================== */
ostream& operator<<(ostream& os, const unique_ptr<DataField>& field) {
    os << "Field(name=\"" << field->getName() 
       << "\", type=\"" << field->getType()
       << "\", length=\"" << field->getLength()
       << endl;
    return os;
}

/* ================== StringField ================== */

void StringField::encodeToBuffer(
    const nlohmann::json& value, vector<uint8_t>& buffer, size_t offset) {

    if (!value.is_string()) {
        throw runtime_error("StringField '" + name + "' expected a string");
    }

    string str = value.get<string>();
    size_t copyLen = std::min(length, str.size());

    memcpy(&buffer[offset], str.data(), copyLen);

    // Pad the rest with nulls if string is shorter than field length
    if (copyLen < length) {
        memset(&buffer[offset + copyLen], 0, length - copyLen);
    }
}

nlohmann::json StringField::decodeFromBuffer(
    const std::vector<uint8_t>& buffer, size_t offset) {

    std::string result(reinterpret_cast<const char*>(&buffer[offset]), length);
    // trim trailing '\0' chars
    result.erase(std::find(result.begin(), result.end(), '\0'), result.end());
    return result;
}

bool StringField::validateValue(const nlohmann::json& value) const {
    if (!value.is_string()) return false;

    return true;
}


/* =============== VarStringField =============== */

void VarStringField::encodeToBuffer(
    const nlohmann::json& value, vector<uint8_t>& buffer, size_t offset) {

    if (!value.is_string()) {
        throw runtime_error("VarStringField '" + name + "' expected a string");
    }

    string str = value.get<string>();

    if (stringBuffer == nullptr) {
        throw runtime_error("Found nulptr when trying to add string to stringBuffer of VarStringField");
    }

    stringStart = stringBuffer->addString(str);
    stringLength = static_cast<uint32_t>(str.length());

    // Write stringStart (8 bytes)
    memcpy(buffer.data() + offset, &stringStart, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    // Write stringLength (4 bytes)
    memcpy(buffer.data() + offset, &stringLength, sizeof(uint32_t));

}

nlohmann::json VarStringField::decodeFromBuffer(
    const std::vector<uint8_t>& buffer, size_t offset) {

    if (!stringBuffer) {
        throw runtime_error("StringBuffer pointer is null in VarStringField");
    }

    
    std::memcpy(&stringStart, buffer.data() + offset, sizeof(stringStart));
    offset += sizeof(stringStart);
    
    std::memcpy(&stringLength, buffer.data() + offset, sizeof(stringLength));
    offset += sizeof(stringLength);

    if (stringStart + stringLength > stringBuffer->getSize()) {
 
        throw std::runtime_error("VarStringField decode error: string offset + length exceeds buffer size");
    }

    const std::vector<uint8_t>& buf = stringBuffer->getBuffer();

    std::string result(reinterpret_cast<const char*>(buf.data() + stringStart), stringLength);

    return nlohmann::json(result);
}

bool VarStringField::validateValue(const nlohmann::json& value) const {
    if (!value.is_string()) return false;

    return true;
}

/* ================== EnumField ================== */

uint8_t EnumField::lookupEnumValue(const string& value) const {
    
    auto it = find(enumValues.begin(), enumValues.end(), value);
    if (it == enumValues.end()) {
        throw invalid_argument("Invalid enum value: " + value);
    }
    return static_cast<uint8_t>(distance(enumValues.begin(), it));
}

void EnumField::encodeToBuffer(
    const nlohmann::json& value, vector<uint8_t>& buffer, size_t offset) {

    if (!value.is_string()) {
        throw runtime_error("EnumField '" + name + "' expected a string");
    }

    uint32_t enumValue = lookupEnumValue(value.get<string>());

    // Write the enum value to the buffer based on the configured byte size
    for (size_t i = 0; i < storageSize; ++i) {
        buffer[offset + i] = static_cast<uint8_t>((enumValue >> (8 * i)) & 0xFF);
    }
}

nlohmann::json EnumField::decodeFromBuffer(
    const std::vector<uint8_t>& buffer, size_t offset) {

    uint32_t enumValue = 0;
    for (size_t i = 0; i < storageSize; ++i) {
        enumValue |= (static_cast<uint32_t>(buffer[offset + i]) << (8 * i));
    }
    if (enumValue >= enumValues.size()) {
        throw std::runtime_error("Invalid enum value in buffer");
    }
    return enumValues[enumValue];
}

bool EnumField::validateValue(const nlohmann::json& value) const {
    if (!value.is_string()) return false;

    return find(enumValues.begin(), enumValues.end(), value.get<string>()) != enumValues.end();
}


/* =============== FloatField =============== */

size_t FloatField::getLength() const {
    if (format == "float32") return 4;
    if (format == "float64") return 8;
    throw std::runtime_error("Unsupported float format: " + format);
}

void FloatField::encodeToBuffer(
    const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) {
    
    if (value.is_null()) return;
    
    if (!value.is_number()) {
        throw std::runtime_error("FloatField '" + name + "' expected a number");
    }
    
    float floatValue = value.get<float>();
    
    if (format == "float32") {
        memcpy(&buffer[offset], &floatValue, sizeof(float));
    } else if (format == "float64") {
        double doubleValue = value.get<double>();
        memcpy(&buffer[offset], &doubleValue, sizeof(double));
    } else {
        throw std::runtime_error("Unsupported float format: " + format);
    }
}

nlohmann::json FloatField::decodeFromBuffer(
    const std::vector<uint8_t>& buffer, size_t offset) {
    
    if (format == "float32") {
        float value;
        memcpy(&value, &buffer[offset], sizeof(float));
        return value;
    } else if (format == "float64") {
        double value;
        memcpy(&value, &buffer[offset], sizeof(double));
        return value;
    } else {
        throw std::runtime_error("Unsupported float format: " + format);
    }
}

bool FloatField::validateValue(const nlohmann::json& value) const {
    if (!value.is_number()) {
        return false;
    }
    
    double floatValue = value.get<double>();

    
    if (minValue.has_value() && floatValue < minValue.value()) {
        return false;
    }
    
    if (maxValue.has_value() && floatValue > maxValue.value()) {
        return false;
    }
    
    return true;
}

/* =============== ArrayField =============== */

ArrayField::ArrayField(std::string name, const nlohmann::json& itemDef, 
                       size_t minItems, size_t maxItems)
    : DataField(name, "array"), minItems(minItems), maxItems(maxItems) {
    
    // Parse the item definition to create the item field
    std::string itemType = itemDef["type"];
    std::string itemName = "item"; // Generic name for array items
    
    if (itemType == "number" && itemDef.contains("format")) {
        std::string format = itemDef["format"];

        std::optional<int64_t> minValue = std::nullopt;
        std::optional<int64_t> maxValue = std::nullopt;

        if (itemDef.contains("minimum")) {
            minValue = itemDef["minimum"];
        }

        if (itemDef.contains("maximum")) {
            maxValue = itemDef["maximum"];
        }

        itemField = std::make_unique<FloatField>(itemName, format, minValue, maxValue);
    } else if (itemType == "integer" && itemDef.contains("format")) {
        std::string format = itemDef["format"];
        IntegerFormatInfo integerFormat = IntegerField::parseIntegerFormat(format);

        std::optional<int64_t> minValue = std::nullopt;
        std::optional<int64_t> maxValue = std::nullopt;

        if (itemDef.contains("minimum")) {
            minValue = itemDef["minimum"];
        }

        if (itemDef.contains("maximum")) {
            maxValue = itemDef["maximum"];
        }

        itemField = std::make_unique<IntegerField>(itemName, integerFormat, minValue, maxValue);


    } else if (itemType == "string") {
        // For string arrays, we'll use StringField with a fixed length
        size_t length = itemDef.value("length", 32); // Default to 32 bytes
        itemField = std::make_unique<StringField>(itemName, "string", length);
    } else {
        throw std::runtime_error("Unsupported array item type: " + itemType);
    }
}

size_t ArrayField::getLength() const {
    return 2 + (itemField->getLength() * maxItems); // 2 bytes for length + max possible items
}

void ArrayField::encodeToBuffer(
    const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) {
    
    if (!value.is_array()) {
        throw std::runtime_error("ArrayField '" + name + "' expected an array");
    }
    
    const auto& array = value.get<std::vector<nlohmann::json>>();
    
    if (array.size() < minItems || array.size() > maxItems) {
        throw std::runtime_error("ArrayField '" + name + "' size " + 
                               std::to_string(array.size()) + " not in range [" + 
                               std::to_string(minItems) + "," + std::to_string(maxItems) + "]");
    }
    
    // Store the actual array length as the first item
    uint16_t actualLength = static_cast<uint16_t>(array.size());
    buffer[offset] = static_cast<uint8_t>(actualLength & 0xFF);
    buffer[offset + 1] = static_cast<uint8_t>((actualLength >> 8) & 0xFF);
    
    size_t itemOffset = offset + 2; // Skip the length field
    for (const auto& item : array) {
        itemField->encodeToBuffer(item, buffer, itemOffset);
        itemOffset += itemField->getLength();
    }
}

nlohmann::json ArrayField::decodeFromBuffer(
    const std::vector<uint8_t>& buffer, size_t offset) {
    
    nlohmann::json array = nlohmann::json::array();
    
    // Read the actual array length from the first 2 bytes
    uint16_t actualLength = static_cast<uint16_t>(buffer[offset]) | 
                           (static_cast<uint16_t>(buffer[offset + 1]) << 8);
    
    size_t itemOffset = offset + 2; // Skip the length field
    
    // Only decode the actual number of items that were stored
    for (size_t i = 0; i < actualLength; ++i) {
        nlohmann::json item = itemField->decodeFromBuffer(buffer, itemOffset);
        array.push_back(item);
        itemOffset += itemField->getLength();
    }
    
    return array;
}

bool ArrayField::validateValue(const nlohmann::json& value) const {

    if (!value.is_array()) return false;

    const auto& array = value.get<std::vector<nlohmann::json>>();

    if (array.size() < minItems || array.size() > maxItems) return false;

    for (const auto& item : array) {
        if (!itemField->validateValue(item)) return false;
    }

    return true;
}



/* =============== IntegerField =============== */

    IntegerFormatInfo IntegerField::parseIntegerFormat(const std::string& format) {
        if (format == "uint8")  return { false, 1 };
        if (format == "uint16") return { false, 2 };
        if (format == "uint32") return { false, 4 };
        if (format == "int8")   return { true, 1 };
        if (format == "int16")  return { true, 2 };
        if (format == "int32")  return { true, 4 };

        throw std::runtime_error("Unsupported integer format: " + format);
    }

    void IntegerField::encodeToBuffer(
        const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) {
            if (value.is_null()) return;

            // Check if the value is actually a number
            if (!value.is_number()) {
                throw std::runtime_error("IntegerField: expected number, got: " + value.dump());
            }

            int32_t val = value.get<int32_t>(); // generic container for both signed/unsigned
            for (size_t i = 0; i < integerFormat.byteLength; ++i) {
                buffer[offset + i] = static_cast<uint8_t>((val >> (8 * i)) & 0xFF);
            }
        }

    nlohmann::json IntegerField::decodeFromBuffer(
            const std::vector<uint8_t>& buffer, size_t offset) {
            uint32_t rawVal = 0;
            for (size_t i = 0; i < integerFormat.byteLength; ++i) {
                rawVal |= static_cast<uint32_t>(buffer[offset + i]) << (8 * i);
            }

            if (integerFormat.isSigned) {
                // Interpret the value as signed
                if (integerFormat.byteLength == 1) return static_cast<int8_t>(rawVal);
                if (integerFormat.byteLength == 2) return static_cast<int16_t>(rawVal);
                if (integerFormat.byteLength == 4) return static_cast<int32_t>(rawVal);
            }

            return rawVal; // return as unsigned
        }

    bool IntegerField::validateValue(const nlohmann::json& value) const {
        if (!value.is_number_integer()) {
            return false;
        }
        
        // Use the existing isSigned flag!
        if (!integerFormat.isSigned && value.get<int64_t>() < 0) {
            return false;  // Unsigned field can't have negative values
        }
        
        int64_t intValue = value.get<int64_t>();
        
        // Apply min/max constraints
        if (minValue.has_value() && intValue < minValue.value()) {
            return false;
        }
        
        if (maxValue.has_value() && intValue > maxValue.value()) {
            return false;
        }
        
        return true;
    }


/* ================== ObjectField ================== */

size_t ObjectField::getLength() const {
    size_t total = 0;
    for (const auto& field : subFields) {
        total += field->getLength();
    }
    return total;
}

void ObjectField::encodeToBuffer(
    const nlohmann::json& value, vector<uint8_t>& buffer, size_t offset) {

    if (!value.is_object()) {
        throw std::runtime_error("ObjectField '" + name + "' expected a JSON object");
    }

    size_t subOffset = offset;
    for (const auto& field : subFields) {
        const std::string& subName = field->getName();
        if (value.contains(subName)) {
            field->encodeToBuffer(value[subName], buffer, subOffset);
        } else {


            // TODO: Handle optional fields or throw if required
            // Example:
            // if (field->isRequired()) throw runtime_error("Missing required field " + subName);
            // else zero-fill or skip
        }
        subOffset += field->getLength();
    }
}

nlohmann::json ObjectField::decodeFromBuffer(
    const std::vector<uint8_t>& buffer, size_t offset) {

    nlohmann::json obj = nlohmann::json::object();
    size_t subOffset = offset;
    for (const auto& field : subFields) {
        obj[field->getName()] = field->decodeFromBuffer(buffer, subOffset);
        subOffset += field->getLength();
    }
    return obj;
}

void ObjectField::addSubField(unique_ptr<DataField> field) {

    subFields.push_back(std::move(field));
}

bool ObjectField::validateValue(const nlohmann::json& value) const {

    if (!value.is_object()) return false;

    // First check if all required fields are present
    for (const auto& requiredField : requiredFields) {
        if (!value.contains(requiredField)) {
    
            return false;  // Missing required field
        }
    }

    // Then validate all subfields that are present
    for (const auto& field : subFields) {
        const std::string& fieldName = field->getName();
        if (value.contains(fieldName)) {
            if (!field->validateValue(value[fieldName])) {
                return false;  // Subfield validation failed
            }
        }
    }

    return true;
}

