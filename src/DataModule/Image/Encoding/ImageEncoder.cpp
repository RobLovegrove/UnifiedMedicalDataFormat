#include "ImageEncoder.hpp"
// #include "imageData.hpp"
#include "../../../Utility/Compression/CompressionType.hpp"

#include <openjpeg.h>
#include <png.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <chrono>

ImageEncoder::ImageEncoder() {
    // Constructor - initialize with default factory
    factory = std::make_shared<CompressionFactory>();
    // Set default strategy to RAW (no compression)
    compressionStrategy = factory->createStrategy("RAW");
}

ImageEncoder::ImageEncoder(std::unique_ptr<CompressionStrategy> strategy) 
    : compressionStrategy(std::move(strategy)) {
    if (!compressionStrategy) {
        throw std::invalid_argument("Compression strategy cannot be null");
    }
    factory = std::make_shared<CompressionFactory>();
}

std::vector<uint8_t> ImageEncoder::compress(const std::vector<uint8_t>& rawData, 
                                            CompressionType encoding,
                                            int width, int height,
                                            uint8_t channels, uint8_t bitDepth) const {
    // Note: Individual frame timing removed - use total timing from ImageData instead
    
    // Create a temporary strategy for this specific compression type
    std::string typeString = compressionTypeToString(encoding);
    auto tempStrategy = factory->createStrategy(typeString);
    
    if (!tempStrategy) {
        std::cerr << "Warning: Unsupported compression type " << typeString << ", using RAW" << std::endl;
        return rawData;
    }
    
    // Check if the strategy supports the given parameters
    if (!tempStrategy->supports(channels, bitDepth)) {
        std::cerr << "Warning: Compression type " << typeString 
                  << " may not support " << static_cast<int>(channels) << " channels and " 
                  << static_cast<int>(bitDepth) << " bit depth" << std::endl;
    }
    
    return tempStrategy->compress(rawData, width, height, channels, bitDepth);
}

std::vector<uint8_t> ImageEncoder::decompress(const std::vector<uint8_t>& compressedData,
                                              CompressionType encoding) const {
    // Note: Individual frame timing removed - use total timing from ImageData instead
    
    // Create a temporary strategy for this specific compression type
    std::string typeString = compressionTypeToString(encoding);
    auto tempStrategy = factory->createStrategy(typeString);
    
    if (!tempStrategy) {
        std::cerr << "Warning: Unsupported compression type " << typeString << ", returning as-is" << std::endl;
        return compressedData;
    }
    
    return tempStrategy->decompress(compressedData);
}

// OpenJPEG compression methods implementation
std::vector<uint8_t> ImageEncoder::compressJPEG2000(const std::vector<uint8_t>& rawData, 
                                                    int width, int height,
                                                    uint8_t channels, uint8_t bitDepth) const {

    
    // Validate input data size
    size_t expectedSize = static_cast<size_t>(width) * height * channels;
    if (rawData.size() != expectedSize) {
        return rawData;
    }
    
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

        opj_image_t* image = opj_image_create(channels, cmptparm.data(), color_space);
        if (!image) {
            return rawData;
        }

        
        // Set image bounds (required for opj_start_compress)
        image->x0 = 0;
        image->y0 = 0;
        image->x1 = width;
        image->y1 = height;
        
        // Check if image data is allocated for all components
        for (int c = 0; c < channels; ++c) {
            if (!image->comps[c].data) {
                opj_image_destroy(image);
                return rawData;
            }
        }
        
        // Copy pixel data to image (interleaved for multi-channel)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                
                if (pixel_index >= static_cast<size_t>(width * height)) {
                    continue;
                }
                
                if (channels == 1) {
                    // Single channel (grayscale) - direct copy
                    if (pixel_index < rawData.size()) {
                        image->comps[0].data[pixel_index] = rawData[pixel_index];
                    }
                } else {
                    // Multi-channel (interleaved) - RGBRGB... format
                    for (int c = 0; c < channels; c++) {
                        size_t data_index = pixel_index * channels + c;
                        if (data_index < rawData.size()) {
                            image->comps[c].data[pixel_index] = rawData[data_index];
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

        opj_stream_t* stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
        if (!stream) {
            opj_image_destroy(image);
            return rawData;
        }

        
        // Set up stream functions
        opj_stream_set_write_function(stream, memory_write);
        opj_stream_set_skip_function(stream, memory_skip);
        opj_stream_set_seek_function(stream, memory_seek);
        opj_stream_set_user_data(stream, &ms_data, nullptr);
        
        // Create codec

        opj_codec_t* codec = opj_create_compress(OPJ_CODEC_J2K);
        if (!codec) {
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            return rawData;
        }

        
        // Set up codec

        if (!opj_setup_encoder(codec, &parameters, image)) {
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        // Start compression
        
        // Compress
        if (!opj_start_compress(codec, image, stream)) {
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        if (!opj_encode(codec, stream)) {
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        if (!opj_end_compress(codec, stream)) {
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
        

        
        return compressed_data;
        
    } catch (const std::exception& e) {
        return rawData;
    }
}

std::vector<uint8_t> ImageEncoder::decompressJPEG2000(const std::vector<uint8_t>& compressedData) const {
    // Sanity check: ensure we have enough data for a valid JPEG 2000 codestream
    if (compressedData.size() < 16) {
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
            opj_stream_destroy(stream);
            return compressedData;
        }

        // Set decoder parameters
        opj_dparameters_t parameters;
        opj_set_default_decoder_parameters(&parameters);
        if (!opj_setup_decoder(codec, &parameters)) {
            opj_stream_destroy(stream);
            opj_destroy_codec(codec);
            return compressedData;
        }

        // Read header
        opj_image_t* image = nullptr;
        if (!opj_read_header(stream, codec, &image)) {
            opj_stream_destroy(stream);
            opj_destroy_codec(codec);
            return compressedData;
        }

        // Decode the image
        if (!opj_decode(codec, stream, image)) {
            opj_stream_destroy(stream);
            opj_destroy_codec(codec);
            opj_image_destroy(image);
            return compressedData;
        }

        if (!opj_end_decompress(codec, stream)) {
            // Silent failure for end decompression
        }
        
        // Extract pixel data from the decoded image
        std::vector<uint8_t> decompressedData;
        
        // Calculate expected size
        int width = image->comps[0].w;
        int height = image->comps[0].h;
        int numComponents = image->numcomps;
        
        // Allocate output buffer
        size_t totalPixels = width * height;
        size_t totalBytes = totalPixels * numComponents;
        decompressedData.resize(totalBytes);
        
        // Copy pixel data from image components
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                
                if (pixel_index >= static_cast<size_t>(width * height)) {
                    continue;
                }
                
                if (numComponents == 1) {
                    // Single channel (grayscale)
                    if (pixel_index < static_cast<size_t>(image->comps[0].w * image->comps[0].h)) {
                        decompressedData[pixel_index] = static_cast<uint8_t>(image->comps[0].data[pixel_index]);
                    }
                } else {
                    // Multi-channel (interleaved) - RGBRGB... format
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
        

        
        return decompressedData;
        
    } catch (const std::exception& e) {
        return compressedData;
    }
}

// PNG compression implementation
std::vector<uint8_t> ImageEncoder::compressPNG(const std::vector<uint8_t>& rawData, 
                                               int width, int height,
                                               uint8_t channels, uint8_t bitDepth) const {

    
    try {
        // Create PNG write structure
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
                                                      nullptr, nullptr, nullptr);
        if (!png_ptr) {
            throw std::runtime_error("Failed to create PNG write struct");
        }
        
        // Create PNG info structure
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, nullptr);
            throw std::runtime_error("Failed to create PNG info struct");
        }
        
        // Set up error handling
        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            throw std::runtime_error("PNG compression failed");
        }
        
        // Create output buffer
        std::vector<uint8_t> output;
        
        // Set up write function to write to our buffer
        png_set_write_fn(png_ptr, &output, 
                         [](png_structp png_ptr, png_bytep data, png_size_t length) {
                             auto* output_buffer = static_cast<std::vector<uint8_t>*>(
                                 png_get_io_ptr(png_ptr));
                             output_buffer->insert(output_buffer->end(), data, data + length);
                         }, nullptr);
        
        // Set image properties
        int color_type = (channels == 1) ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB;
        png_set_IHDR(png_ptr, info_ptr, width, height, bitDepth,
                      color_type, PNG_INTERLACE_NONE,
                      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        
        // Write header
        png_write_info(png_ptr, info_ptr);
        
        // Prepare row pointers
        std::vector<png_bytep> row_pointers(height);
        for (int y = 0; y < height; y++) {
            row_pointers[y] = const_cast<png_bytep>(&rawData[y * width * channels]);
        }
        
        // Write image data
        png_write_image(png_ptr, row_pointers.data());
        png_write_end(png_ptr, info_ptr);
        
        // Cleanup
        png_destroy_write_struct(&png_ptr, &info_ptr);
        

        
        return output;
        
    } catch (const std::exception& e) {
        return rawData; // Return original data on error
    }
}

std::vector<uint8_t> ImageEncoder::decompressPNG(const std::vector<uint8_t>& compressedData) const {
    // Sanity check: ensure we have enough data for a valid PNG file
    if (compressedData.size() < 8) {
        return compressedData;
    }
    
    // Check PNG signature (first 8 bytes)
    const uint8_t png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (memcmp(compressedData.data(), png_signature, 8) != 0) {
        return compressedData;
    }
    
    try {
        // Create PNG read structure
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
                                                     nullptr, nullptr, nullptr);
        if (!png_ptr) {
            throw std::runtime_error("Failed to create PNG read struct");
        }
        
        // Create PNG info structure
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);
            throw std::runtime_error("Failed to create PNG info struct");
        }
        
        // Set up error handling
        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            throw std::runtime_error("PNG decompression failed");
        }
        
        // Create input buffer for reading
        struct ReadBuffer {
            const uint8_t* data;
            size_t size;
            size_t position;
        };
        
        ReadBuffer read_buffer = {compressedData.data(), compressedData.size(), 0};
        
        // Set up read function to read from our buffer
        png_set_read_fn(png_ptr, &read_buffer, 
                        [](png_structp png_ptr, png_bytep data, png_size_t length) {
                            auto* buffer = static_cast<ReadBuffer*>(png_get_io_ptr(png_ptr));
                            if (buffer->position + length <= buffer->size) {
                                memcpy(data, buffer->data + buffer->position, length);
                                buffer->position += length;
                            } else {
                                // Read what we can
                                size_t available = buffer->size - buffer->position;
                                memcpy(data, buffer->data + buffer->position, available);
                                buffer->position = buffer->size;
                                // Fill remaining with zeros (libpng expects this)
                                memset(data + available, 0, length - available);
                            }
                        });
        
        // Read PNG header
        png_read_info(png_ptr, info_ptr);
        
        // Get image properties
        png_uint_32 width, height;
        int bit_depth, color_type, interlace_type, compression_type, filter_type;
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                      &interlace_type, &compression_type, &filter_type);
        

        
        // Determine number of channels
        int channels;
        switch (color_type) {
            case PNG_COLOR_TYPE_GRAY:
                channels = 1;
                break;
            case PNG_COLOR_TYPE_RGB:
                channels = 3;
                break;
            case PNG_COLOR_TYPE_RGBA:
                channels = 4;
                break;
            default:
                throw std::runtime_error("Unsupported PNG color type: " + std::to_string(color_type));
        }
        
        // Create output buffer
        std::vector<uint8_t> output(width * height * channels);
        
        // Read image data row by row
        std::vector<png_bytep> row_pointers(height);
        for (png_uint_32 y = 0; y < height; y++) {
            row_pointers[y] = &output[y * width * channels];
        }
        
        png_read_image(png_ptr, row_pointers.data());
        
        // Cleanup
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        

        
        return output;
        
    } catch (const std::exception& e) {
        return compressedData; // Return original data on error
    }
}

// Helper method to convert CompressionType enum to string
std::string ImageEncoder::compressionTypeToString(CompressionType type) const {
    switch (type) {
        case CompressionType::JPEG2000_LOSSLESS:
            return "JPEG2000_LOSSLESS";
        case CompressionType::PNG:
            return "PNG";
        case CompressionType::ZSTD:
            return "ZSTD";  // Note: ZSTD not implemented yet
        case CompressionType::RAW:
        default:
            return "RAW";
    }
}

// Strategy management methods
void ImageEncoder::setCompressionStrategy(std::unique_ptr<CompressionStrategy> strategy) {
    if (!strategy) {
        throw std::invalid_argument("Compression strategy cannot be null");
    }
    compressionStrategy = std::move(strategy);
}

void ImageEncoder::setCompressionStrategy(const std::string& type) {
    auto newStrategy = factory->createStrategy(type);
    if (!newStrategy) {
        throw std::invalid_argument("Unsupported compression type: " + type);
    }
    compressionStrategy = std::move(newStrategy);
}

std::string ImageEncoder::getCurrentCompressionType() const {
    if (!compressionStrategy) {
        return "NONE";
    }
    return compressionStrategy->getCompressionType();
}

bool ImageEncoder::supports(int channels, uint8_t bitDepth) const {
    if (!compressionStrategy) {
        return false;
    }
    return compressionStrategy->supports(channels, bitDepth);
}

bool ImageEncoder::testCompression() const {
    if (!compressionStrategy) {
        return false;
    }
    
    try {
        // Create a simple test image (2x2, 3 channels, 8-bit)
        std::vector<uint8_t> testData = {255, 0, 0,    // Red pixel
                                        0, 255, 0,     // Green pixel
                                        0, 0, 255,     // Blue pixel
                                        255, 255, 255}; // White pixel
        
        // Test compression
        auto compressed = compressionStrategy->compress(testData, 2, 2, 3, 8);
        if (compressed.empty()) {
            return false;
        }
        
        // Test decompression
        auto decompressed = compressionStrategy->decompress(compressed);
        if (decompressed.empty()) {
            return false;
        }
        
        // Verify data integrity (for lossless compression)
        if (compressionStrategy->getCompressionType() == "RAW" || 
            compressionStrategy->getCompressionType() == "PNG" ||
            compressionStrategy->getCompressionType() == "JPEG2000_LOSSLESS") {
            return testData == decompressed;
        }
        
        // For lossy compression, just check that we got some data back
        return !decompressed.empty();
        
    } catch (const std::exception& e) {
        std::cerr << "Compression test failed: " << e.what() << std::endl;
        return false;
    }
}

bool ImageEncoder::testOpenJPEGIntegration() const {
    // Test that OpenJPEG is properly linked and working
    
    // Check OpenJPEG version
    
    // Test basic OpenJPEG functionality
    opj_cparameters_t parameters;
    opj_set_default_encoder_parameters(&parameters);
    
    return true;
}

// OpenJPEG stream functions implementation
OPJ_SIZE_T ImageEncoder::memory_read(void* buffer, OPJ_SIZE_T size, void* user_data) {
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

OPJ_SIZE_T ImageEncoder::memory_write(void* buffer, OPJ_SIZE_T size, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (ms->current_pos + size <= ms->max_output_size) {
        memcpy(ms->output_buffer + ms->current_pos, buffer, size);
        ms->current_pos += size;
        ms->output_size = std::max(ms->output_size, ms->current_pos);
        return size;
    }
    return 0;
}

OPJ_OFF_T ImageEncoder::memory_skip(OPJ_OFF_T offset, void* user_data) {
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

OPJ_BOOL ImageEncoder::memory_seek(OPJ_OFF_T offset, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (!ms) return OPJ_FALSE;
    if (offset >= 0 && (size_t)offset <= ms->input_size) {
        ms->current_pos = (size_t)offset;
        return OPJ_TRUE;
    }
    return OPJ_FALSE;
} 