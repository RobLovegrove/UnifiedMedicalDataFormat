#include "imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataModule.hpp"
#include "../../Xref/xref.hpp"

#include "string"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <openjpeg.h>

// OpenJPEG event handlers for debugging
void opj_info_callback(const char* msg, void* client_data) { 
    std::cout << "[openjpeg info] " << msg; 
}
void opj_warn_callback(const char* msg, void* client_data) { 
    std::cout << "[openjpeg warn] " << msg; 
}
void opj_error_callback(const char* msg, void* client_data) { 
    std::cout << "[openjpeg err ] " << msg; 
}

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
    // Set decompression flag based on current encoding
    frame->needsDecompression = (encoding != ImageEncoding::RAW);
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
    
    // Debug: Print encoding at start of writeData
    std::cout << "=== ImageData::writeData() called ===" << std::endl;
    std::cout << "Encoding: " << encodingToString(encoding) << std::endl;
    std::cout << "Number of frames: " << frames.size() << std::endl;
    
    // Write each frame as embedded data (not as separate modules)
    for (size_t i = 0; i < frames.size(); i++) {
        // Debug: Print current encoding
        std::cout << "Frame " << i << " - Current encoding: " << encodingToString(encoding) << std::endl;
        
        // Check if we need to compress this frame
        if (encoding != ImageEncoding::RAW) {
            std::cout << "Compression needed for frame " << i << std::endl;
            // Compress the frame's pixel data
            if (encoding == ImageEncoding::JPEG2000_LOSSLESS) {
                std::cout << "Compressing frame " << i << " with JPEG 2000 lossless..." << std::endl;
                // Get width and height from dimensions array
                int frameWidth = dimensions.size() > 0 ? dimensions[0] : 16;
                int frameHeight = dimensions.size() > 1 ? dimensions[1] : 16;
                frames[i]->pixelData = compressJPEG2000(frames[i]->pixelData, frameWidth, frameHeight);
                
                // Update the frame's data size after compression
                frames[i]->header->setDataSize(frames[i]->pixelData.size());
            } else if (encoding == ImageEncoding::PNG) {
                std::cout << "TODO: Compressing frame " << i << " with PNG..." << std::endl;
                // TODO: Implement PNG compression
                // frames[i]->pixelData = compressPNG(frames[i]->pixelData, frameWidth, frameHeight);
            }
        } else {
            std::cout << "No compression needed for frame " << i << " (RAW encoding)" << std::endl;
        }
        
        XRefTable tempXref;
        frames[i]->writeBinary(out, tempXref);
    }
    
    streampos endPos = out.tellp();

    uint64_t totalDataSize = endPos - startPos;

    header->setDataSize(totalDataSize);

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
    
    std::cout << "=== ImageData::getModuleSpecificData() called ===" << std::endl;
    std::cout << "Encoding: " << encodingToString(encoding) << std::endl;
    std::cout << "Number of frames: " << frames.size() << std::endl;
    
    for (size_t i = 0; i < frames.size(); i++) {
        const auto& frame = frames[i];
        std::cout << "Frame " << i << " - needsDecompression: " << (frame->needsDecompression ? "true" : "false") << std::endl;
        
        // Check if frame needs decompression and hasn't been decompressed yet
        if (frame->needsDecompression) {
            std::cout << "Decompressing frame " << i << "..." << std::endl;
            // Decompress the frame's pixel data in-place
            std::vector<uint8_t> decompressedData = decompressFrameData(frame->pixelData);
            frame->pixelData = decompressedData; // Overwrite with decompressed data
            frame->needsDecompression = false; // Mark as decompressed
            std::cout << "Frame " << i << " decompressed: " << frame->pixelData.size() << " bytes" << std::endl;
        } else {
            std::cout << "Frame " << i << " does not need decompression" << std::endl;
        }
        
        // Get the frame's data with schema (now decompressed)
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

// Memory stream data structure for OpenJPEG
struct MemoryStreamData {
    const uint8_t* input_buffer;        // Input data for reading
    uint8_t* output_buffer;             // Output buffer for writing
    size_t input_size;                  // Size of input data
    size_t output_size;                 // Size of output data
    size_t current_pos;                 // Current position in stream
    size_t max_output_size;             // Maximum output buffer size
};

// Custom stream read function for OpenJPEG
OPJ_SIZE_T memory_read(void* buffer, OPJ_SIZE_T size, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (!ms || !ms->input_buffer) return 0;
    size_t bytes_left = ms->input_size - ms->current_pos;
    size_t bytes_to_read = std::min((size_t)size, bytes_left);
    if (bytes_to_read > 0) {
        memcpy(buffer, ms->input_buffer + ms->current_pos, bytes_to_read);
        ms->current_pos += bytes_to_read;
        return (OPJ_SIZE_T)bytes_to_read;
    }
    return 0; // EOF
}

// Custom stream write function for OpenJPEG
OPJ_SIZE_T memory_write(void* buffer, OPJ_SIZE_T size, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (ms->current_pos + size <= ms->max_output_size) {
        memcpy(ms->output_buffer + ms->current_pos, buffer, size);
        ms->current_pos += size;
        ms->output_size = std::max(ms->output_size, ms->current_pos);
        return size;
    }
    return 0;
}

// Custom stream skip function for OpenJPEG
OPJ_OFF_T memory_skip(OPJ_OFF_T offset, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (!ms) return -1;

    // Use signed arithmetic to check bounds
    long long new_pos = (long long)ms->current_pos + (long long)offset;
    if (new_pos >= 0 && (size_t)new_pos <= ms->input_size) {
        ms->current_pos = (size_t)new_pos;
        return offset;
    }
    return -1;
}

// Custom stream seek function for OpenJPEG
OPJ_BOOL memory_seek(OPJ_OFF_T offset, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (!ms) return OPJ_FALSE;
    if (offset >= 0 && (size_t)offset <= ms->input_size) {
        ms->current_pos = (size_t)offset;
        return OPJ_TRUE;
    }
    return OPJ_FALSE;
}

// OpenJPEG compression methods implementation
std::vector<uint8_t> ImageData::compressJPEG2000(const std::vector<uint8_t>& rawData, 
                                                  int width, int height) const {
    std::cout << "Compressing " << rawData.size() << " bytes to JPEG 2000 lossless..." << std::endl;
    std::cout << "Image: " << width << "x" << height 
              << " with " << (int)channels << " channels, " 
              << (int)bitDepth << "-bit depth" << std::endl;
    
    try {
        // Set up compression parameters
        opj_cparameters_t parameters;
        opj_set_default_encoder_parameters(&parameters);
        parameters.tcp_numlayers = 1;
        parameters.cp_disto_alloc = 1;
        parameters.tcp_rates[0] = 0;  // Lossless compression
        parameters.irreversible = 0;   // Lossless mode
        
        // Calculate optimal number of resolutions based on image size
        int max_possible = 1;
        int min_dim = std::min(width, height);
        while (min_dim > 1) {
            max_possible++;
            min_dim /= 2;
        }
        // Don't count the 1x1 level as a resolution
        max_possible = std::max(1, max_possible - 1);
        // Use maximum, but cap at 6 for very large images
        int num_resolutions = std::min(max_possible, 6);
        parameters.numresolution = num_resolutions;
        
        std::cout << "Using " << num_resolutions << " resolutions for " 
                  << width << "x" << height << " image with " << (int)channels << " channels" << std::endl;
        
        // Create image component parameters for all channels
        std::vector<opj_image_cmptparm_t> cmptparm(channels);
        for (int c = 0; c < channels; ++c) {
            cmptparm[c].dx = 1;
            cmptparm[c].dy = 1;
            cmptparm[c].w = width;
            cmptparm[c].h = height;
            cmptparm[c].prec = bitDepth;
            cmptparm[c].bpp = bitDepth;
            cmptparm[c].sgnd = 0;  // unsigned
        }
        
        // Pick colorspace based on channels
        OPJ_COLOR_SPACE color_space = (channels == 1) ? OPJ_CLRSPC_GRAY : OPJ_CLRSPC_SRGB;
        
        // Create image
        std::cout << "Creating OpenJPEG image with " << (int)channels << " components..." << std::endl;
        opj_image_t* image = opj_image_create(channels, cmptparm.data(), color_space);
        if (!image) {
            std::cout << "❌ Failed to create OpenJPEG image" << std::endl;
            return rawData;
        }
        std::cout << "✅ OpenJPEG image created successfully" << std::endl;
        
        // Set image bounds (required for opj_start_compress)
        image->x0 = 0;
        image->y0 = 0;
        image->x1 = width;
        image->y1 = height;
        
        // Check if image data is allocated for all components
        for (int c = 0; c < channels; ++c) {
            if (!image->comps[c].data) {
                std::cout << "❌ Image component " << c << " data not allocated" << std::endl;
                opj_image_destroy(image);
                return rawData;
            }
        }
        std::cout << "✅ Image data allocated for all " << (int)channels << " components" << std::endl;
        
        // Copy pixel data to image (interleaved for multi-channel)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                if (pixel_index < rawData.size()) {
                    // For single channel, just copy directly
                    if (channels == 1) {
                        image->comps[0].data[y * width + x] = rawData[pixel_index];
                    } else {
                        // For multi-channel, assume interleaved data (RGBRGB...)
                        for (int c = 0; c < channels; c++) {
                            size_t data_index = pixel_index * channels + c;
                            if (data_index < rawData.size()) {
                                image->comps[c].data[y * width + x] = rawData[data_index];
                            }
                        }
                    }
                }
            }
        }
        
        // Create output memory stream
        std::vector<uint8_t> output_buffer(rawData.size() * 2);  // Start with 2x size
        MemoryStreamData ms_data = {
            nullptr,                    // No input data for compression
            output_buffer.data(),       // Output buffer
            0,                          // No input size
            0,                          // Output size starts at 0
            0,                          // Current position starts at 0
            output_buffer.size()        // Max output size
        };
        
        // Create OpenJPEG stream
        std::cout << "Creating OpenJPEG stream..." << std::endl;
        opj_stream_t* stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
        if (!stream) {
            std::cout << "❌ Failed to create OpenJPEG stream" << std::endl;
            opj_image_destroy(image);
            return rawData;
        }
        std::cout << "✅ OpenJPEG stream created successfully" << std::endl;
        
        // Set up stream functions
        opj_stream_set_write_function(stream, memory_write);
        opj_stream_set_skip_function(stream, memory_skip);
        opj_stream_set_seek_function(stream, memory_seek);
        opj_stream_set_user_data(stream, &ms_data, nullptr);
        
        // Create codec
        std::cout << "Creating OpenJPEG codec..." << std::endl;
        opj_codec_t* codec = opj_create_compress(OPJ_CODEC_J2K);
        if (!codec) {
            std::cout << "❌ Failed to create OpenJPEG codec" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            return rawData;
        }
        std::cout << "✅ OpenJPEG codec created successfully" << std::endl;
        
        // Set up codec
        std::cout << "Setting up OpenJPEG encoder..." << std::endl;
        if (!opj_setup_encoder(codec, &parameters, image)) {
            std::cout << "❌ Failed to setup OpenJPEG encoder" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        std::cout << "✅ OpenJPEG encoder setup successfully" << std::endl;
        
        // Start compression
        std::cout << "Starting OpenJPEG compression..." << std::endl;
        
        // Compress
        if (!opj_start_compress(codec, image, stream)) {
            std::cout << "❌ Failed to start OpenJPEG compression" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        std::cout << "✅ OpenJPEG compression started successfully" << std::endl;
        
        if (!opj_encode(codec, stream)) {
            std::cout << "Failed to encode with OpenJPEG" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        if (!opj_end_compress(codec, stream)) {
            std::cout << "Failed to end OpenJPEG compression" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        // Clean up
        opj_stream_destroy(stream);
        opj_image_destroy(image);
        opj_destroy_codec(codec);
        
        // Return compressed data
        std::vector<uint8_t> compressed_data(ms_data.output_buffer, 
                                           ms_data.output_buffer + ms_data.output_size);
        
        std::cout << "Compression complete: " << rawData.size() << " -> " 
                  << compressed_data.size() << " bytes (" 
                  << (100.0 * compressed_data.size() / rawData.size()) << "% of original)" << std::endl;
        
        // Debug: Print first few bytes of compressed data
        std::cout << "Compressed data structure (first 32 bytes): ";
        for (size_t i = 0; i < std::min(compressed_data.size(), size_t(32)); i++) {
            printf("%02X ", compressed_data[i]);
        }
        if (compressed_data.size() > 32) {
            std::cout << "...";
        }
        std::cout << std::endl;
        
        return compressed_data;
        
    } catch (const std::exception& e) {
        std::cout << "OpenJPEG compression error: " << e.what() << std::endl;
        return rawData;
    }
}

std::vector<uint8_t> ImageData::decompressJPEG2000(const std::vector<uint8_t>& compressedData) const {
    std::cout << "Decompressing " << compressedData.size() << " bytes from JPEG 2000 lossless..." << std::endl;
    
    // Sanity check: ensure we have enough data for a valid JPEG 2000 codestream
    if (compressedData.size() < 16) {
        std::cout << "❌ Compressed data too small for valid JPEG 2000 codestream" << std::endl;
        return compressedData;
    }
    
    try {
        // Create input memory stream
        MemoryStreamData ms_data = {
            compressedData.data(),        // Input data
            nullptr,                      // No output buffer for reading
            compressedData.size(),        // Input data size
            0,                           // No output size for reading
            0,                           // Current position starts at 0
            0                            // No max output size for reading
        };
        
        // Create OpenJPEG stream for reading
        opj_stream_t* stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
        if (!stream) {
            std::cout << "❌ Failed to create OpenJPEG stream for decompression" << std::endl;
            return compressedData;
        }
        
        // Set up stream functions for reading
        opj_stream_set_read_function(stream, memory_read);
        opj_stream_set_skip_function(stream, memory_skip);
        opj_stream_set_seek_function(stream, memory_seek);
        opj_stream_set_user_data(stream, &ms_data, nullptr);
        opj_stream_set_user_data_length(stream, ms_data.input_size);
        
        // Create decoder
        opj_codec_t* codec = opj_create_decompress(OPJ_CODEC_J2K);
        if (!codec) {
            std::cout << "❌ Failed to create OpenJPEG decoder" << std::endl;
            opj_stream_destroy(stream);
            return compressedData;
        }

        // Attach handlers for debugging
        opj_set_info_handler(codec, opj_info_callback, nullptr);
        opj_set_warning_handler(codec, opj_warn_callback, nullptr);
        opj_set_error_handler(codec, opj_error_callback, nullptr);

        // NEW: Set decoder parameters
        opj_dparameters_t parameters;
        opj_set_default_decoder_parameters(&parameters);
        if (!opj_setup_decoder(codec, &parameters)) {
            std::cout << "❌ Failed to setup OpenJPEG decoder" << std::endl;
            opj_stream_destroy(stream);
            opj_destroy_codec(codec);
            return compressedData;
        }

        // Read header
        opj_image_t* image = nullptr;
        if (!opj_read_header(stream, codec, &image)) {
            std::cout << "❌ Failed to read JPEG 2000 header" << std::endl;
            opj_stream_destroy(stream);
            opj_destroy_codec(codec);
            return compressedData;
        }

        std::cout << "✅ Successfully read JPEG 2000 header" << std::endl;
        std::cout << "Image dimensions: " << image->comps[0].w << "x" << image->comps[0].h 
                  << " with " << image->numcomps << " components" << std::endl;

        // Decode the image
        std::cout << "Attempting to decode JPEG 2000 image..." << std::endl;
        if (!opj_decode(codec, stream, image)) {
            std::cout << "❌ Failed to decode JPEG 2000 image" << std::endl;
            opj_stream_destroy(stream);
            opj_destroy_codec(codec);
            opj_image_destroy(image);
            return compressedData;
        }
        std::cout << "✅ Successfully decoded JPEG 2000 image" << std::endl;

        if (!opj_end_decompress(codec, stream)) {
            std::cout << "❌ Failed to end decompression" << std::endl;
        }
        
        // Extract pixel data from the decoded image
        std::vector<uint8_t> decompressedData;
        
        // Calculate expected size
        int width = image->comps[0].w;
        int height = image->comps[0].h;
        int numComponents = image->numcomps;
        
        std::cout << "Decompressed image: " << width << "x" << height 
                  << " with " << numComponents << " components" << std::endl;
        
        // Allocate output buffer
        size_t totalPixels = width * height;
        size_t totalBytes = totalPixels * numComponents;
        decompressedData.resize(totalBytes);
        
        // Copy pixel data from image components
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                
                if (numComponents == 1) {
                    // Single channel (grayscale)
                    if (pixel_index < static_cast<size_t>(image->comps[0].w * image->comps[0].h)) {
                        decompressedData[pixel_index] = static_cast<uint8_t>(image->comps[0].data[pixel_index]);
                    }
                } else {
                    // Multi-channel (interleaved)
                    for (int c = 0; c < numComponents; c++) {
                        size_t data_index = pixel_index * numComponents + c;
                        if (data_index < decompressedData.size() && pixel_index < static_cast<size_t>(image->comps[c].w * image->comps[c].h)) {
                            decompressedData[data_index] = static_cast<uint8_t>(image->comps[c].data[pixel_index]);
                        }
                    }
                }
            }
        }
        
        // Clean up
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        
        std::cout << "Decompression complete: " << compressedData.size() << " -> " 
                  << decompressedData.size() << " bytes" << std::endl;
        
        return decompressedData;
        
    } catch (const std::exception& e) {
        std::cout << "OpenJPEG decompression error: " << e.what() << std::endl;
        return compressedData;
    }
}

std::vector<uint8_t> ImageData::decompressFrameData(const std::vector<uint8_t>& compressedData) const {
    switch (encoding) {
        case ImageEncoding::JPEG2000_LOSSLESS:
            return decompressJPEG2000(compressedData);
        case ImageEncoding::PNG:
            // TODO: Implement PNG decompression
            std::cout << "PNG decompression not yet implemented" << std::endl;
            return compressedData;
        default:
            return compressedData; // Already uncompressed
    }
}

bool ImageData::testOpenJPEGIntegration() const {
    // Test that OpenJPEG is properly linked and working
    std::cout << "Testing OpenJPEG integration..." << std::endl;
    
    // Check OpenJPEG version
    std::cout << "OpenJPEG version: " << opj_version() << std::endl;
    
    // Test basic OpenJPEG functionality
    opj_cparameters_t parameters;
    opj_set_default_encoder_parameters(&parameters);
    
    std::cout << "OpenJPEG encoder parameters initialized successfully" << std::endl;
    
    return true;
}

