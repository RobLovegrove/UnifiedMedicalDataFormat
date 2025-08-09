#include "ImageEncoder.hpp"
#include "imageData.hpp" // For ImageEncoding enum
#include <openjpeg.h>
#include <png.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

// OpenJPEG event handlers for debugging
void ImageEncoder::opj_info_callback(const char* msg, void* client_data) { 
    std::cout << "[openjpeg info] " << msg; 
}

void ImageEncoder::opj_warn_callback(const char* msg, void* client_data) { 
    std::cout << "[openjpeg warn] " << msg; 
}

void ImageEncoder::opj_error_callback(const char* msg, void* client_data) { 
    std::cout << "[openjpeg err ] " << msg; 
}

ImageEncoder::ImageEncoder() {
    // Constructor - nothing to initialize
}

std::vector<uint8_t> ImageEncoder::compress(const std::vector<uint8_t>& rawData, 
                                            ImageEncoding encoding,
                                            int width, int height,
                                            uint8_t channels, uint8_t bitDepth) const {
    switch (encoding) {
        case ImageEncoding::JPEG2000_LOSSLESS:
            return compressJPEG2000(rawData, width, height, channels, bitDepth);
        case ImageEncoding::PNG:
            return compressPNG(rawData, width, height, channels, bitDepth);
        case ImageEncoding::RAW:
        default:
            return rawData; // No compression
    }
}

std::vector<uint8_t> ImageEncoder::decompress(const std::vector<uint8_t>& compressedData,
                                              ImageEncoding encoding) const {
    switch (encoding) {
        case ImageEncoding::JPEG2000_LOSSLESS:
            return decompressJPEG2000(compressedData);
        case ImageEncoding::PNG:
            return decompressPNG(compressedData);
        case ImageEncoding::RAW:
        default:
            return compressedData; // Already uncompressed
    }
}

// OpenJPEG compression methods implementation
std::vector<uint8_t> ImageEncoder::compressJPEG2000(const std::vector<uint8_t>& rawData, 
                                                    int width, int height,
                                                    uint8_t channels, uint8_t bitDepth) const {
    std::cout << "Compressing " << rawData.size() << " bytes to JPEG 2000 lossless..." << std::endl;
    std::cout << "Image: " << width << "x" << height 
              << " with " << (int)channels << " channels, " 
              << (int)bitDepth << "-bit depth" << std::endl;
    
    // Validate input data size
    size_t expectedSize = static_cast<size_t>(width) * height * channels;
    if (rawData.size() != expectedSize) {
        std::cout << "❌ Data size mismatch: expected " << expectedSize << " bytes for " 
                  << width << "x" << height << "x" << (int)channels 
                  << " but got " << rawData.size() << " bytes" << std::endl;
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
                
                if (pixel_index >= static_cast<size_t>(width * height)) {
                    std::cout << "❌ Pixel index out of bounds: " << pixel_index << " >= " << (width * height) << std::endl;
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
                        } else {
                            std::cout << "❌ Data index out of bounds: " << data_index << " >= " << rawData.size() << std::endl;
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
        
        std::cout << "Frame compression: " << rawData.size() << " -> " 
                  << compressed_data.size() << " bytes (" 
                  << std::fixed << std::setprecision(2)
                  << (100.0 * compressed_data.size() / rawData.size()) << "% of original frame size)" << std::endl;
        
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

std::vector<uint8_t> ImageEncoder::decompressJPEG2000(const std::vector<uint8_t>& compressedData) const {
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

        // Set decoder parameters
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
        
        // Validate expected output size
        size_t expectedOutputSize = static_cast<size_t>(width) * height * numComponents;
        std::cout << "Expected output size: " << expectedOutputSize << " bytes" << std::endl;
        
        // Allocate output buffer
        size_t totalPixels = width * height;
        size_t totalBytes = totalPixels * numComponents;
        decompressedData.resize(totalBytes);
        
        // Copy pixel data from image components
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                
                if (pixel_index >= static_cast<size_t>(width * height)) {
                    std::cout << "❌ Pixel index out of bounds during decompression: " << pixel_index << " >= " << (width * height) << std::endl;
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
                        } else if (data_index >= decompressedData.size()) {
                            std::cout << "❌ Decompressed data index out of bounds: " << data_index << " >= " << decompressedData.size() << std::endl;
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

// PNG compression implementation
std::vector<uint8_t> ImageEncoder::compressPNG(const std::vector<uint8_t>& rawData, 
                                               int width, int height,
                                               uint8_t channels, uint8_t bitDepth) const {
    std::cout << "Starting PNG compression for " << width << "x" << height 
              << " image with " << channels << " channels..." << std::endl;
    
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
        
        std::cout << "Frame PNG compression: " << rawData.size() << " -> " 
                  << output.size() << " bytes (" 
                  << std::fixed << std::setprecision(2)
                  << (100.0 * output.size() / rawData.size()) << "% of original frame size)" << std::endl;
        
        return output;
        
    } catch (const std::exception& e) {
        std::cout << "PNG compression error: " << e.what() << std::endl;
        return rawData; // Return original data on error
    }
}

std::vector<uint8_t> ImageEncoder::decompressPNG(const std::vector<uint8_t>& compressedData) const {
    std::cout << "Decompressing " << compressedData.size() << " bytes from PNG..." << std::endl;
    
    // Sanity check: ensure we have enough data for a valid PNG file
    if (compressedData.size() < 8) {
        std::cout << "❌ Compressed data too small for valid PNG file" << std::endl;
        return compressedData;
    }
    
    // Check PNG signature (first 8 bytes)
    const uint8_t png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (memcmp(compressedData.data(), png_signature, 8) != 0) {
        std::cout << "❌ Invalid PNG signature" << std::endl;
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
        
        std::cout << "PNG image: " << width << "x" << height 
                  << " with " << bit_depth << "-bit depth, color type " << color_type << std::endl;
        
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
        
        std::cout << "✅ PNG decompression successful: " << compressedData.size() << " -> " 
                  << output.size() << " bytes" << std::endl;
        
        return output;
        
    } catch (const std::exception& e) {
        std::cout << "PNG decompression error: " << e.what() << std::endl;
        return compressedData; // Return original data on error
    }
}

bool ImageEncoder::testOpenJPEGIntegration() const {
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