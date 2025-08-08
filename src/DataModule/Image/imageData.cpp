#include "imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataModule.hpp"
#include "../../Xref/xref.hpp"

#include "string"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

// Encoding conversion utilities implementation
std::optional<ImageEncoding> stringToEncoding(const std::string& str) {
    if (str == "raw") return ImageEncoding::RAW;
    if (str == "jpeg-ls-lossless") return ImageEncoding::JPEG_LS_LOSSLESS;
    if (str == "png") return ImageEncoding::PNG;
    return std::nullopt;  // Invalid encoding
}

std::string encodingToString(ImageEncoding encoding) {
    switch (encoding) {
        case ImageEncoding::RAW: return "raw";
        case ImageEncoding::JPEG_LS_LOSSLESS: return "jpeg-ls-lossless";
        case ImageEncoding::PNG: return "png";
        default: return "raw";  // Fallback
    }
}

ImageData::ImageData(const string& schemaPath, UUID uuid) : DataModule(schemaPath, uuid, ModuleType::Image) {
    // Initialize encoding to RAW by default (always safe for medical data)
    encoding = ImageEncoding::RAW;
    initialise();
}

void ImageData::parseDataSchema(const nlohmann::json& schemaJson) {
    if (!schemaJson.contains("properties")) {
        throw runtime_error("Schema missing essential 'properties' field.");
    }

    const auto& props = schemaJson["properties"];

    for (const auto& [name, definition] : props.items()) {
        fields.emplace_back(parseField(name, definition, rowSize));
    }
}

void ImageData::addData(std::unique_ptr<FrameData> frame) {
    frames.push_back(std::move(frame));
}

void ImageData::addMetaData(const nlohmann::json& data) {
    // Call base class implementation first
    DataModule::addMetaData(data);
    
    // Extract dimensions from metadata and store in C++ array
    if (data.contains("dimensions") && data["dimensions"].is_array()) {
        dimensions.clear();
        for (const auto& dim : data["dimensions"]) {
            dimensions.push_back(dim.get<uint16_t>());
        }
    }
    
    // Extract encoding from metadata and store in C++ member
    if (data.contains("encoding")) {
        std::string enc_str = data["encoding"];
        auto encoding_opt = stringToEncoding(enc_str);
        if (encoding_opt.has_value()) {
            encoding = encoding_opt.value();
        } else {
            // Invalid encoding - log warning and use default
            std::cerr << "Warning: Invalid encoding '" << enc_str 
                      << "' in metadata, using RAW encoding" << std::endl;
            encoding = ImageEncoding::RAW;
        }
    }
}

int ImageData::getDepth() const {
    return dimensions.size() >= 3 ? dimensions[2] : 0;
}

void ImageData::decodeMetadataRows(std::istream& in, size_t actualDataSize) {
    // Call base class to decode metadata normally
    DataModule::decodeMetadataRows(in, actualDataSize);
    
    // Now extract dimensions and encoding from the decoded metadata
    dimensions.clear();
    
    // Get the decoded metadata and find the dimensions and encoding fields
    auto metadata = getMetadataAsJson();
    
    if (metadata.is_array() && !metadata.empty()) {
        // Take the first row (assuming all rows have the same dimensions and encoding)
        auto firstRow = metadata[0];
        
        if (firstRow.contains("dimensions") && firstRow["dimensions"].is_array()) {
            for (const auto& dim : firstRow["dimensions"]) {
                dimensions.push_back(dim.get<uint16_t>());
            }
        }
        
        // Extract encoding from decoded metadata
        if (firstRow.contains("encoding")) {
            std::string enc_str = firstRow["encoding"];
            auto encoding_opt = stringToEncoding(enc_str);
            if (encoding_opt.has_value()) {
                encoding = encoding_opt.value();
            } else {
                // Invalid encoding - log warning and use default
                std::cerr << "Warning: Invalid encoding '" << enc_str 
                          << "' in decoded metadata, using RAW encoding" << std::endl;
                encoding = ImageEncoding::RAW;
            }
        } else {
            // No encoding in metadata, use default
            encoding = ImageEncoding::RAW;
        }
    }
}

void ImageData::writeData(std::ostream& out) const {
    streampos startPos = out.tellp();
    
    // Write each frame as embedded data (not as separate modules)
    for (size_t i = 0; i < frames.size(); i++) {
        XRefTable tempXref;
        frames[i]->writeBinary(out, tempXref);
    }
    
    streampos endPos = out.tellp();

    uint64_t totalDataSize = endPos - startPos;

    header->setDataSize(totalDataSize);

}

void ImageData::writeStringBuffer(std::ostream& out) {
    if (stringBuffer.getSize() != 0) {
        stringBuffer.writeToFile(out);
    }
}

void ImageData::decodeData(std::istream& in, size_t) {
    // Clear any existing frames
    frames.clear();
    
    // Get frame count from C++ dimensions array
    int depth = getDepth();
    
    // Read each frame as a complete DataModule using fromStream
    for (int i = 0; i < depth; i++) {
        // Get current position
        std::streampos frameStart = in.tellg();
        
        // Read frame header to get frame size (this advances the stream)
        DataHeader frameHeader;

        frameHeader.readDataHeader(in);

        // Calculate frame size
        uint64_t frameSize = frameHeader.getModuleSize();
        
        // Reset to frame start
        in.seekg(frameStart);
        
        // Read the entire frame (including header) into a buffer
        std::vector<char> frameBuffer(frameSize);
        in.read(frameBuffer.data(), frameSize);

        
        // Create stringstream from the complete frame data
        std::istringstream frameStream(std::string(frameBuffer.begin(), frameBuffer.end()));
        
        // Use DataModule::fromStream to read the frame
        auto frame = std::unique_ptr<FrameData>(static_cast<FrameData*>(
            DataModule::fromStream(frameStream, 0, static_cast<uint8_t>(ModuleType::Frame)).release()
        ));
        
        frames.push_back(std::move(frame));

        in.seekg(frameStart + static_cast<std::streamoff>(frameSize));
    }
}

void ImageData::printData(std::ostream& out) const {
    out << "ImageData with " << frames.size() << " frames:" << std::endl;
    
    for (size_t i = 0; i < frames.size(); i++) {
        out << "Frame " << i << ": ";
        frames[i]->printData(out);
    }
}

// Override the virtual method for image-specific data
std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
ImageData::getModuleSpecificData() const {
    // Return the frames as a vector of ModuleData
    std::vector<ModuleData> frameDataArray;
    
    for (const auto& frame : frames) {
        // Get the frame's data with schema
        ModuleData frameData = frame->getDataWithSchema();
        frameDataArray.push_back(frameData);
    }
    
    return frameDataArray;  // Returns vector of ModuleData for frames
}

// Encoding methods implementation
void ImageData::setEncoding(ImageEncoding enc) {
    encoding = enc;
}

ImageEncoding ImageData::getEncoding() const {
    return encoding;
}

std::string ImageData::getEncodingString() const {
    return encodingToString(encoding);
}

bool ImageData::setEncodingFromString(const std::string& enc_str) {
    auto encoding_opt = stringToEncoding(enc_str);
    if (encoding_opt.has_value()) {
        encoding = encoding_opt.value();
        return true;  // Success
    } else {
        return false;  // Invalid encoding
    }
}

bool ImageData::validateEncodingInSchema() const {
    // Check if the schema defines an encoding field with valid enum values
    if (schemaJson.contains("properties") && 
        schemaJson["properties"].contains("metadata") &&
        schemaJson["properties"]["metadata"].contains("properties") &&
        schemaJson["properties"]["metadata"]["properties"].contains("encoding")) {
        
        const auto& encodingDef = schemaJson["properties"]["metadata"]["properties"]["encoding"];
        
        // Check if the schema has valid enum values
        if (encodingDef.contains("enum") && encodingDef["enum"].is_array()) {
            for (const auto& enumVal : encodingDef["enum"]) {
                if (!stringToEncoding(enumVal).has_value()) {
                    return false;  // Invalid enum value found
                }
            }
            return true;  // All enum values are valid
        }
    }
    return true;  // No encoding field in schema, which is valid
}

