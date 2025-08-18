#include "imageData.hpp"
#include "ImageEncoder.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataModule.hpp"
#include "../../Xref/xref.hpp"
#include "../ModuleData.hpp"

#include "string"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem> // Required for filesystem::exists

using namespace std;

// Encoding conversion utilities implementation
std::optional<ImageEncoding> stringToEncoding(const std::string& str) {
    if (str == "raw") return ImageEncoding::RAW;
    if (str == "jpeg2000-lossless") return ImageEncoding::JPEG2000_LOSSLESS;
    if (str == "png") return ImageEncoding::PNG;
    return std::nullopt;  // Invalid encoding
}

std::string encodingToString(ImageEncoding encoding) {
    switch (encoding) {
        case ImageEncoding::RAW: return "raw";
        case ImageEncoding::JPEG2000_LOSSLESS: return "jpeg2000-lossless";
        case ImageEncoding::PNG: return "png";
        default: return "raw";  // Fallback
    }
}

ImageData::ImageData(const string& schemaPath, UUID uuid) : DataModule(schemaPath, uuid, ModuleType::Image) {
    // Initialize encoding to RAW by default (always safe for medical data)
    encoding = ImageEncoding::RAW;
    
    // Initialize the image encoder
    encoder = std::make_unique<ImageEncoder>();

    initialise();
}

ImageData::ImageData(
    const string& schemaPath, const nlohmann::json& schemaJson, UUID uuid) 
    : DataModule(schemaPath, schemaJson, uuid, ModuleType::Image) {

    // Initialize encoding to RAW by default (always safe for medical data)
    encoding = ImageEncoding::RAW;
    
    // Initialize the image encoder
    encoder = std::make_unique<ImageEncoder>();

    initialise();
}

void ImageData::parseDataSchema(const nlohmann::json& schemaJson) {
    // Debug: Print the schema being parsed
    std::cout << "ImageData::parseDataSchema called with schema:" << std::endl;
    std::cout << schemaJson.dump(2) << std::endl;
    
    // Parse the data section using standard JSON Schema $ref
    if (schemaJson.contains("properties") && schemaJson["properties"].contains("frames")) {
        const auto& framesProp = schemaJson["properties"]["frames"];
        
        if (framesProp.contains("$ref")) {
            // Extract the frame schema reference path directly
            frameSchemaPath = framesProp["$ref"];
            std::cout << "Frame schema path extracted: " << frameSchemaPath << std::endl;
        } else {
            throw runtime_error("Image schema data section missing valid $ref to frame schema");
        }
    } else {
        throw runtime_error("Image schema missing required 'frames' property in data section");
    }
}

// struct ModuleData {
//     nlohmann::json metadata;
//     std::variant<
//         nlohmann::json,                    // For tabular data
//         std::vector<uint8_t>,              // For frame pixel data
//         std::vector<ModuleData>            // For N-dimensional data
//     > data;
// };

void ImageData::addData(
    const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>& moduleData) {

    // Load the frame schema if not already loaded
    if (frameSchemaPath.empty()) {
        throw std::runtime_error("Frame schema path not set. Call parseDataSchema first.");
    }

    // Validate schema path exists
    if (!std::filesystem::exists(frameSchemaPath)) {
        throw std::runtime_error("Frame schema file not found: " + frameSchemaPath);
    }

    // Validate that we're receiving frame data (nested modules)
    if (!std::holds_alternative<std::vector<ModuleData>>(moduleData)) {
        throw std::runtime_error("ImageData::addData expects frame data (vector<ModuleData>), but received different data type");
    }

    // Extract the frame data
    const auto& data = std::get<std::vector<ModuleData>>(moduleData);

    // Validate frame count
    if (data.empty()) {
        throw std::runtime_error("ImageData::addData received empty frame data");
    }

    if (static_cast<int>(data.size()) != getFrameCount()) {
        throw std::runtime_error("ImageData::addData: Number of frames does not match frame count");
    }

    // Start timing for total compression
    auto compressionStart = std::chrono::high_resolution_clock::now();

    cout << "Constructing " << data.size() << " frames" << endl;

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& frame = data[i];
        auto frameModule = std::make_unique<FrameData>(frameSchemaPath, UUID());

        // Validate frame structure against schema
        if (!frame.metadata.contains("position") || !frame.metadata.contains("orientation")) {
            throw std::runtime_error("Frame " + std::to_string(i) + " missing required metadata (position/orientation)");
        }

        // Validate position array dimensions match image dimensions
        const auto& position = frame.metadata["position"];
        if (position.size() != dimensions.size()) {
            throw std::runtime_error("Frame " + std::to_string(i) + 
                " position dimensions (" + std::to_string(position.size()) + 
                ") don't match image dimensions (" + std::to_string(dimensions.size()) + ")");
        }

        // Validate orientation vectors are 3D for 2D images
        if (dimensions.size() == 2) {
            if (!frame.metadata["orientation"].contains("row_cosine") || 
                !frame.metadata["orientation"].contains("column_cosine")) {
                throw std::runtime_error("Frame " + std::to_string(i) + " missing required orientation vectors");
            }
            
            const auto& rowCos = frame.metadata["orientation"]["row_cosine"];
            const auto& colCos = frame.metadata["orientation"]["column_cosine"];
            
            if (rowCos.size() != 3 || colCos.size() != 3) {
                throw std::runtime_error("Frame " + std::to_string(i) + " orientation vectors must be 3D");
            }
        }

        // Validate timestamp format (basic ISO 8601 check)
        if (frame.metadata.contains("timestamp")) {
            const std::string& timestamp = frame.metadata["timestamp"];
            if (timestamp.length() != 20) {
                throw std::runtime_error("Frame " + std::to_string(i) + " timestamp must be 20 characters (ISO 8601)");
            }
        }

        // Validate frame number is sequential
        if (frame.metadata.contains("frame_number")) {
            int frameNum = frame.metadata["frame_number"];
            if (frameNum < 0 || frameNum >= static_cast<int>(data.size())) {
                throw std::runtime_error("Frame " + std::to_string(i) + 
                    " frame_number out of range: " + std::to_string(frameNum));
            }
        }

        // Extract frame-specific data (assuming it's binary pixel data)
        if (!std::holds_alternative<std::vector<uint8_t>>(frame.data)) {
            throw std::runtime_error("Frame " + std::to_string(i) + " data is not binary pixel data");
        }
        
        const auto& pixelData = std::get<std::vector<uint8_t>>(frame.data);

        // Calculate expected size for this frame (2D slice)
        // Each frame is a 2D slice, so we only use the first 2 dimensions
        size_t bytesPerPixel = (bitDepth + 7) / 8; // Round up for non-byte-aligned bit depths
        size_t expectedSize = dimensions[0] * dimensions[1] * channels * bytesPerPixel;

        if (pixelData.size() != expectedSize) {
            throw std::runtime_error("Frame " + std::to_string(i) + 
                " pixel data size mismatch. Expected: " + std::to_string(expectedSize) + 
                ", Got: " + std::to_string(pixelData.size()) + 
                " (dimensions: " + std::to_string(dimensions[0]) + "x" + std::to_string(dimensions[1]) + 
                ", channels: " + std::to_string(channels) + ", bitDepth: " + std::to_string(bitDepth) + ")");
        }

        // Validate bit depth is reasonable
        if (bitDepth < 1 || bitDepth > 64) {
            throw std::runtime_error("Frame " + std::to_string(i) + " invalid bit depth: " + std::to_string(bitDepth));
        }

        // Validate channel count is reasonable
        if (channels < 1 || channels > 16) {
            throw std::runtime_error("Frame " + std::to_string(i) + " invalid channel count: " + std::to_string(channels));
        }

        // Set the frame metadata
        frameModule->addMetaData(frame.metadata);

        // Set the pixel data
        frameModule->addData(frame.data);
        
        // Set decompression flag based on current encoding
        frameModule->needsDecompression = (encoding != ImageEncoding::RAW);
        
        // Add to frames collection
        frames.push_back(std::move(frameModule));
    }

    // Post-processing validation: ensure frame consistency
    if (frames.size() > 1) {
        // Check that all frames have consistent metadata structure
        const auto& firstFrame = frames[0];
        for (size_t i = 1; i < frames.size(); ++i) {
            // Validate that all frames have the same required fields
            if (frames[i]->getMetadataAsJson().size() != firstFrame->getMetadataAsJson().size()) {
                throw std::runtime_error("Inconsistent metadata structure across frames");
            }
        }
    }

    // End timing and output total compression time
    auto compressionEnd = std::chrono::high_resolution_clock::now();
    auto compressionDuration = std::chrono::duration_cast<std::chrono::microseconds>(compressionEnd - compressionStart);
    
    std::cout << "Total compression time for " << data.size() << " frames (" 
              << (encoding == ImageEncoding::JPEG2000_LOSSLESS ? "JPEG2000" : 
                  encoding == ImageEncoding::PNG ? "PNG" : "RAW") << "): " 
              << compressionDuration.count() << " microseconds" << std::endl;

}

void ImageData::addMetaData(const nlohmann::json& data) {
    
    if (data.is_array()) {
        // ImageData only supports single metadata row
        throw std::runtime_error("ImageData::addMetaData: Only single metadata row supported. Received array with " + 
                                std::to_string(data.size()) + " rows. Image modules should have one metadata row per module.");
    }
    
    if (!data.is_object()) {
        throw std::runtime_error("ImageData::addMetaData: Invalid metadata format. Metadata must be a JSON object.");
    }

    // Call base class implementation first to populate the fields vector
    DataModule::addMetaData(data);
    
    dimensions.clear();
    dimensionNames.clear();

    // // Check if contains image_structure format
    if (data.contains("image_structure") && data["image_structure"].is_object()) {
        const auto& imageStruct = data["image_structure"];

          
        // Extract dimensions array first (required field)
        if (!imageStruct.contains("dimensions") || !imageStruct["dimensions"].is_array()) {
            throw std::runtime_error("ImageData: 'dimensions' array is required in image_structure");
        }
        
        const auto& dimsArray = imageStruct["dimensions"];
        if (dimsArray.size() < 2) {
            throw std::runtime_error("ImageData: 'dimensions' array must have at least 2 elements (width, height)");
        }
        
        // Must have first dimension
        if (!dimsArray[0].is_number()) {
            throw std::runtime_error("ImageData: first dimension must be a number, got: " + dimsArray[0].dump());
        }
        dimensions.push_back(dimsArray[0].get<uint16_t>());

        if (!dimsArray[1].is_number()) {
            // If no second dimension, set it to 1
            // Ensure no other dimensions are set
            if (dimsArray.size() > 2) {
                throw std::runtime_error("ImageData: second dimension must be a number, got: " + dimsArray[1].dump());
            }
            dimensions.push_back(1);
        } else {
            dimensions.push_back(dimsArray[1].get<uint16_t>());
        }
        
        // Add all dimensions to our internal array
        for (size_t i = 2; i < dimsArray.size(); ++i) {
            if (!dimsArray[i].is_number()) {
                throw std::runtime_error("ImageData: dimension " + std::to_string(i) + " must be a number, got: " + dimsArray[i].dump());
            }
            dimensions.push_back(dimsArray[i].get<uint16_t>());
        }
        
        // Extract dimension names from the dimension_names array if available
        if (imageStruct.contains("dimension_names") && imageStruct["dimension_names"].is_array()) {
            const auto& namesArray = imageStruct["dimension_names"];
            
            // Ensure we have names for all dimensions
            if (namesArray.size() >= dimensions.size()) {
                for (size_t i = 0; i < dimensions.size(); ++i) {
                    dimensionNames.push_back(namesArray[i].get<std::string>());
                }
            } else {
                // Fallback: use default names for missing ones
                for (size_t i = 0; i < dimensions.size(); ++i) {
                    if (i < namesArray.size()) {
                        dimensionNames.push_back(namesArray[i].get<std::string>());
                    } else {
                        // Generate default names for missing dimensions
                        if (i == 0) dimensionNames.push_back("x");
                        else if (i == 1) dimensionNames.push_back("y");
                        else dimensionNames.push_back("dim" + std::to_string(i));
                    }
                }
            }
        } else {
            // No dimension names provided, generate defaults
            for (size_t i = 0; i < dimensions.size(); ++i) {
                if (i == 0) dimensionNames.push_back("x");
                else if (i == 1) dimensionNames.push_back("y");
                else dimensionNames.push_back("dim" + std::to_string(i));
            }
        }
        
        // Extract encoding from image_structure and store in C++ member
        if (imageStruct.contains("encoding")) {
            std::string enc_str = imageStruct["encoding"];
            auto encoding_opt = stringToEncoding(enc_str);
            if (encoding_opt.has_value()) {
                encoding = encoding_opt.value();
            } else {
                // Invalid encoding - log warning and use default
                std::cerr << "Warning: Invalid encoding '" << enc_str 
                          << "' in image_structure, using RAW encoding" << std::endl;
                encoding = ImageEncoding::RAW;
            }
        }
        
        // Extract bit depth from image_structure
        if (imageStruct.contains("bit_depth")) {
            bitDepth = imageStruct["bit_depth"].get<uint8_t>();
        }

        if (imageStruct.contains("channels")) {
            channels = imageStruct["channels"].get<uint8_t>();
        }
    }   
}

int ImageData::getFrameCount() const {
    if (dimensions.size() <= 2) return 1;  // 2D image
    
    // Multiply all dimensions except first two (width, height)
    int totalFrames = 1;
    for (size_t i = 2; i < dimensions.size(); ++i) {
        totalFrames *= dimensions[i];
    }
    return totalFrames;
}

std::vector<uint16_t> ImageData::getNonZeroDimensions() const {
    std::vector<uint16_t> nonZeroDims;
    
    // Add all non-zero dimensions
    for (size_t i = 0; i < dimensions.size(); ++i) {
        if (dimensions[i] > 0) {
            nonZeroDims.push_back(dimensions[i]);
        }
    }
    
    return nonZeroDims;
}

std::vector<std::string> ImageData::getNonZeroDimensionNames() const {
    std::vector<std::string> nonZeroNames;
    
    // Add names for all non-zero dimensions
    for (size_t i = 0; i < dimensions.size() && i < dimensionNames.size(); ++i) {
        if (dimensions[i] > 0) {
            nonZeroNames.push_back(dimensionNames[i]);
        }
    }
    
    return nonZeroNames;
}

FieldMap buildFieldMap(
    const std::vector<uint8_t>& rowBuffer,
    const std::vector<std::unique_ptr<DataField>>& fields
) {
    FieldMap map;
    
    // Build flattened field list including nested fields (same logic as readTableRows)
    std::vector<std::pair<std::string, DataField*>> flattenedFields;
    
    for (const auto& field : fields) {
        if (auto* objectField = dynamic_cast<ObjectField*>(field.get())) {
            // Add nested fields with dot notation
            const auto& nestedFields = objectField->getNestedFields();
            for (const auto& nestedField : nestedFields) {
                std::string fieldPath = field->getName() + "." + nestedField->getName();
                flattenedFields.push_back({fieldPath, nestedField.get()});
            }
        } else {
            // Regular field
            flattenedFields.push_back({field->getName(), field.get()});
        }
    }
    
    size_t numFlattenedFields = flattenedFields.size();
    size_t bitmapSize = (numFlattenedFields + 7) / 8;
    
    if (rowBuffer.size() < bitmapSize) {
        throw std::runtime_error("Row buffer too small to contain bitmap");
    }
    
    const uint8_t* bitmap = rowBuffer.data();
    size_t offset = bitmapSize;
    
    for (size_t i = 0; i < numFlattenedFields; ++i) {
        bool present = bitmap[i / 8] & (1 << (i % 8));
        
        const auto& [fieldPath, fieldPtr] = flattenedFields[i];
        
        FieldInfo info;
        info.present = present;
        info.offset = offset;
        info.field = fieldPtr;
        info.length = present ? fieldPtr->getLength() : 0;
        
        if (present) {
            // Add flattened field to map
            map[fieldPath] = info;
            offset += info.length;
        } else {
            // Field not present, but still add to map for consistency
            map[fieldPath] = info;
        }
    }
    
    return map;
}

void ImageData::readMetadataRows(std::istream& in) {

    // Call base class to decode metadata normally
    DataModule::readMetadataRows(in);
    // Now extract dimensions and encoding from the decoded metadata
    dimensions.clear();
    dimensionNames.clear();

    auto fieldMap = buildFieldMap(metaDataRows[0], metaDataFields);     
    // Get dimensions
    if (fieldMap["image_structure.dimensions"].present) {
        auto& f = fieldMap["image_structure.dimensions"];

        // Use the polymorphic decodeFromBuffer method
        nlohmann::json dimsJson = f.field->decodeFromBuffer(metaDataRows[0], f.offset);

        // Convert JSON array to std::vector<int>
        if (dimsJson.is_array()) {
            for (auto& val : dimsJson) {
                if (val.is_number_unsigned()) {
                    dimensions.push_back(val.get<uint16_t>());
                }
            }
        }
    }
    else {
        // Essential dimensions field not present
        throw runtime_error("Essential dimensions field is not present");
    }

    {

        if (!fieldMap["image_structure.dimension_names"].present) {
            // Essential dimensions names field not present
            throw runtime_error("Essential dimension names field is not present");
        }

        // Get dimension names
        auto it = fieldMap.find("image_structure.dimension_names");
        if (it != fieldMap.end() && it->second.present) {
            FieldInfo& f = it->second;
            auto& rowBuffer = metaDataRows[0]; // vector<uint8_t> of the first row
            size_t offset = f.offset;          // start of this field in the buffer

            dimensionNames.clear();

            // Assuming the field stores an array of strings
            size_t endOffset = f.offset + f.length;
            while (offset < endOffset) {
                // decodeFromBuffer reads from 'rowBuffer' starting at 'offset'
                // and returns a JSON object (string) for this element
                auto jsonValue = f.field->decodeFromBuffer(rowBuffer, offset);
                if (jsonValue.is_array()) {
                    for (auto& j : jsonValue) {
                        dimensionNames.push_back(j.get<std::string>());
                    }
                } else if (jsonValue.is_string()) {
                    dimensionNames.push_back(jsonValue.get<std::string>());
                }

                offset += f.length;
            }
        }
    }

    {

        if (!fieldMap["image_structure.encoding"].present) {
            // Essential encoding field not present
            throw runtime_error("Essential encoding field is not present");
        }

        // Get encoding
        auto it = fieldMap.find("image_structure.encoding");
        if (it != fieldMap.end() && it->second.present) {
            FieldInfo& f = it->second;
            auto& rowBuffer = metaDataRows[0]; // first metadata row
            size_t offset = f.offset;

            // Decode the field value from buffer
            std::string enc_str = f.field->decodeFromBuffer(rowBuffer, offset).get<std::string>();

            auto encoding_opt = stringToEncoding(enc_str);
            if (encoding_opt.has_value()) {
                encoding = encoding_opt.value();
                needsDecompression = (encoding != ImageEncoding::RAW);
            } else {
                std::cerr << "Warning: Invalid encoding '" << enc_str
                        << "' in metadata, using RAW encoding" << std::endl;
                encoding = ImageEncoding::RAW;
                needsDecompression = false;
            }
        } else {
            // default if field missing
            encoding = ImageEncoding::RAW;
            needsDecompression = false;
        }
    }

    // Get bit depth
    {

        if (!fieldMap["image_structure.bit_depth"].present) {
            // Essential bit_depth field not present
            throw runtime_error("Essential bit_depth field is not present");
        }
        auto it = fieldMap.find("image_structure.bit_depth");
        if (it != fieldMap.end() && it->second.present) {
            FieldInfo& f = it->second;
            auto& rowBuffer = metaDataRows[0]; // vector<uint8_t>
            size_t offset = f.offset;

            bitDepth = f.field->decodeFromBuffer(rowBuffer, offset).get<uint8_t>();
        } else {
            bitDepth = 8; // default
        }
    }

    // Get channels
    {
        if (!fieldMap["image_structure.channels"].present) {
            // Essential channels field not present
            throw runtime_error("Essential channels field is not present");
        }    
        auto it = fieldMap.find("image_structure.channels");
        if (it != fieldMap.end() && it->second.present) {
            FieldInfo& f = it->second;
            auto& rowBuffer = metaDataRows[0]; // vector<uint8_t>
            size_t offset = f.offset;

            channels = f.field->decodeFromBuffer(rowBuffer, offset).get<uint8_t>();
        } else {
            channels = 1; // default
        }
    }
}

void ImageData::writeData(std::ostream& out) const {
    streampos startPos = out.tellp();
    
    // Write each frame as embedded data (not as separate modules)
    for (size_t i = 0; i < frames.size(); i++) {
        // Check if we need to compress this frame
        if (encoding != ImageEncoding::RAW) {
            // Compress the frame's pixel data
            // Get width and height from dimensions array
            int frameWidth = dimensions.size() > 0 ? dimensions[0] : 16;
            int frameHeight = dimensions.size() > 1 ? dimensions[1] : 16;
            
            // Use the encoder to compress the frame
            frames[i]->pixelData = encoder->compress(frames[i]->pixelData, encoding, 
                                                   frameWidth, frameHeight, channels, bitDepth);
            
            // Update the frame's data size after compression
            frames[i]->header->setDataSize(frames[i]->pixelData.size());
        }
        
        XRefTable tempXref;
        frames[i]->writeBinary(out, tempXref);
    }
    
    streampos endPos = out.tellp();

    uint64_t totalDataSize = endPos - startPos;

    // Calculate and display total compression statistics
    if (encoding != ImageEncoding::RAW) {
        size_t totalOriginalSize = 0;
        size_t totalCompressedSize = 0;
        
        for (const auto& frame : frames) {
            // Estimate original size based on dimensions and channels
            size_t frameOriginalSize = static_cast<size_t>(dimensions[0]) * dimensions[1] * channels;
            totalOriginalSize += frameOriginalSize;
            totalCompressedSize += frame->pixelData.size();
        }
        
        double compressionRatio = 100.0 * totalCompressedSize / totalOriginalSize;
        std::cout << "\n=== COMPRESSION SUMMARY ===" << std::endl;
        std::cout << "Total frames: " << frames.size() << std::endl;
        std::cout << "Total original size: " << totalOriginalSize << " bytes" << std::endl;
        std::cout << "Total compressed size: " << totalCompressedSize << " bytes" << std::endl;
        std::cout << "Overall compression: " << std::fixed << std::setprecision(2) 
                  << compressionRatio << "% of original (" 
                  << (100.0 - compressionRatio) << "% space saved)" << std::endl;
        std::cout << "===========================" << std::endl << std::endl;
    }

    header->setDataSize(totalDataSize);

}

void ImageData::readData(std::istream& in) {

    // Clear any existing frames
    frames.clear();
    
    // Get frame count from C++ dimensions array
    int frameCount = getFrameCount();
    
    // Read each frame as a complete DataModule using fromStream
    for (int i = 0; i < frameCount; i++) {
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

        frame->needsDecompression = needsDecompression;
        frames.push_back(std::move(frame));

        in.seekg(frameStart + static_cast<std::streamoff>(frameSize));
    }
}

void ImageData::printData(std::ostream& out) const {
    out << "ImageData with " << frames.size() << " frames:" << std::endl;
}

// Override the virtual method for image-specific data
std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
ImageData::getModuleSpecificData() const {

    // Return the frames as a vector of ModuleData
    std::vector<ModuleData> frameDataArray;
    
    // Start timing for total decompression
    auto decompressionStart = std::chrono::high_resolution_clock::now();
    
    // Process all frames
    for (size_t i = 0; i < frames.size(); i++) {
        const auto& frame = frames[i];
        
        // Check if frame needs decompression and hasn't been decompressed yet
        if (frame->needsDecompression) {
            // Decompress the frame's pixel data in-place
            std::vector<uint8_t> decompressedData = decompressFrameData(frame->pixelData);
            frame->pixelData = decompressedData; // Overwrite with decompressed data
            frame->needsDecompression = false; // Mark as decompressed
        }
        
        // Get the frame's data with schema (now decompressed)
        ModuleData frameData = frame->getDataWithSchema();
        frameDataArray.push_back(frameData);
    }
    
    // End timing and output total decompression time
    auto decompressionEnd = std::chrono::high_resolution_clock::now();
    auto decompressionDuration = std::chrono::duration_cast<std::chrono::microseconds>(decompressionEnd - decompressionStart);
    
    // Print summary of frame processing with timing
    std::cout << "ImageData: Processed " << frames.size() << " frames with " 
              << encodingToString(encoding) << " encoding" << std::endl;
    std::cout << "Total decompression time for " << frames.size() << " frames (" 
              << encodingToString(encoding) << "): " 
              << decompressionDuration.count() << " microseconds" << std::endl;
    
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
        schemaJson["properties"]["metadata"]["propertiesclear"].contains("encoding")) {
        
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

std::vector<uint8_t> ImageData::decompressFrameData(const std::vector<uint8_t>& compressedData) const {
    if (encoding == ImageEncoding::RAW) {
        return compressedData; // Already uncompressed
    }
    
    // Use the encoder to decompress the frame
    return encoder->decompress(compressedData, encoding);
}





        