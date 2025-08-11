#include "imageData.hpp"
#include "ImageEncoder.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataModule.hpp"
#include "../../Xref/xref.hpp"

#include "string"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>




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
    
    // Initialize image format attributes with defaults
    bitDepth = 8;   // Default 8-bit
    channels = 1;   // Default grayscale
    
    // Initialize the image encoder
    encoder = std::make_unique<ImageEncoder>();
    
    initialise();
}

void ImageData::parseDataSchema(const nlohmann::json& schemaJson) {
    // Extract frame schema reference from data section
    if (schemaJson.contains("$frame_schema")) {
        frameSchemaPath = schemaJson["$frame_schema"];
    } else {
        throw runtime_error("Image schema missing required '$frame_schema' field in data section");
    }
}

void ImageData::addData(std::unique_ptr<FrameData> frame) {
    // Set decompression flag based on current encoding
    frame->needsDecompression = (encoding != ImageEncoding::RAW);
    frames.push_back(std::move(frame));
}

void ImageData::addData(const std::vector<std::pair<nlohmann::json, std::vector<uint8_t>>>& frameDataPairs) {
    // Load the frame schema if not already loaded
    if (frameSchemaPath.empty()) {
        throw std::runtime_error("Frame schema path not set. Call parseDataSchema first.");
    }
    
    // Start timing for total compression
    auto compressionStart = std::chrono::high_resolution_clock::now();
    
    // Create FrameData objects for each pair
    for (const auto& [metadata, pixelData] : frameDataPairs) {

        auto frame = std::make_unique<FrameData>(frameSchemaPath, UUID());

        // Set the frame metadata
        frame->addMetaData(metadata);
        // Set the pixel data
        frame->pixelData = pixelData;
        // Set decompression flag based on current encoding
        frame->needsDecompression = (encoding != ImageEncoding::RAW);
        // Add to frames collection
        frames.push_back(std::move(frame));
    }
    
    // End timing and output total compression time
    auto compressionEnd = std::chrono::high_resolution_clock::now();
    auto compressionDuration = std::chrono::duration_cast<std::chrono::microseconds>(compressionEnd - compressionStart);
    
    std::cout << "Total compression time for " << frameDataPairs.size() << " frames (" 
              << (encoding == ImageEncoding::JPEG2000_LOSSLESS ? "JPEG2000" : 
                  encoding == ImageEncoding::PNG ? "PNG" : "RAW") << "): " 
              << compressionDuration.count() << " microseconds" << std::endl;
}

void ImageData::addMetaData(const nlohmann::json& data) {
    // Call base class implementation first
    DataModule::addMetaData(data);
    
    dimensions.clear();
    dimensionNames.clear();
    
    // Extract dimensions array first (required field)
    if (!data.contains("dimensions") || !data["dimensions"].is_array()) {
        throw std::runtime_error("ImageData: 'dimensions' array is required in metadata");
    }
    
    const auto& dimsArray = data["dimensions"];
    if (dimsArray.size() < 2) {
        throw std::runtime_error("ImageData: 'dimensions' array must have at least 2 elements (width, height)");
    }
    
    // First two elements are width and height
    if (!dimsArray[0].is_number()) {
        throw std::runtime_error("ImageData: first dimension must be a number, got: " + dimsArray[0].dump());
    }
    if (!dimsArray[1].is_number()) {
        throw std::runtime_error("ImageData: second dimension must be a number, got: " + dimsArray[1].dump());
    }
    
    // Add all dimensions to our internal array
    for (size_t i = 0; i < dimsArray.size(); ++i) {
        if (!dimsArray[i].is_number()) {
            throw std::runtime_error("ImageData: dimension " + std::to_string(i) + " must be a number, got: " + dimsArray[i].dump());
        }
        dimensions.push_back(dimsArray[i].get<uint16_t>());
    }
    
    // Extract dimension names from the dimension_names array if available
    if (data.contains("dimension_names") && data["dimension_names"].is_array()) {
        const auto& namesArray = data["dimension_names"];
        
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
    
    // Extract bit depth from metadata
    if (data.contains("bit_depth")) {
        bitDepth = data["bit_depth"].get<uint8_t>();
    }
    
    // Extract channels from metadata (if available)
    if (data.contains("channels")) {
        channels = data["channels"].get<uint8_t>();
    } else {
        // Default to grayscale for medical images
        channels = 1;
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

void ImageData::decodeMetadataRows(std::istream& in, size_t actualDataSize) {
    // Call base class to decode metadata normally
    DataModule::decodeMetadataRows(in, actualDataSize);
    
    // Now extract dimensions and encoding from the decoded metadata
    dimensions.clear();
    dimensionNames.clear();
    
    // Get the decoded metadata and find the dimensions and encoding fields
    auto metadata = getMetadataAsJson();
    
    if (metadata.is_array() && !metadata.empty()) {
        // Take the first row (assuming all rows have the same dimensions and encoding)
        auto firstRow = metadata[0];
        
        // Extract dimensions directly from the dimensions array
        
        // Extract dimensions directly from the dimensions array
        if (firstRow.contains("dimensions") && firstRow["dimensions"].is_array()) {
            const auto& dimsArray = firstRow["dimensions"];
            for (size_t i = 0; i < dimsArray.size(); ++i) {
                uint16_t dim = dimsArray[i].get<uint16_t>();
                if (dim > 0) {  // Only add non-zero dimensions
                    dimensions.push_back(dim);
                }
            }
        }
        
        // Extract dimension names from the dimension_names array if available
        if (firstRow.contains("dimension_names") && firstRow["dimension_names"].is_array()) {
            const auto& namesArray = firstRow["dimension_names"];
            for (size_t i = 0; i < namesArray.size() && i < dimensions.size(); ++i) {
                dimensionNames.push_back(namesArray[i].get<std::string>());
            }
        }
        

        

        
        // Extract encoding from decoded metadata
        if (firstRow.contains("encoding")) {
            std::string enc_str = firstRow["encoding"];
            auto encoding_opt = stringToEncoding(enc_str);
            if (encoding_opt.has_value()) {
                encoding = encoding_opt.value();
                needsDecompression = (encoding != ImageEncoding::RAW);
            } else {
                // Invalid encoding - log warning and use default
                std::cerr << "Warning: Invalid encoding '" << enc_str 
                          << "' in decoded metadata, using RAW encoding" << std::endl;
                encoding = ImageEncoding::RAW;
                needsDecompression = false;
            }
        } else {
            // No encoding in metadata, use default
            encoding = ImageEncoding::RAW;
            needsDecompression = false;
        }
        
        // Extract bit depth from decoded metadata
        if (firstRow.contains("bit_depth")) {
            bitDepth = firstRow["bit_depth"].get<uint8_t>();
        }
        
        // Extract channels from decoded metadata
        if (firstRow.contains("channels")) {
            channels = firstRow["channels"].get<uint8_t>();
        } else {
            // Default to grayscale for medical images
            channels = 1;
        }
        
        // Update decompression flags for all frames based on new encoding
        for (auto& frame : frames) {
            frame->needsDecompression = (encoding != ImageEncoding::RAW);
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

void ImageData::decodeData(std::istream& in, size_t) {
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





        