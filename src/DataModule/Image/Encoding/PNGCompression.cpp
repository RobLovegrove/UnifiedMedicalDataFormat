#include "PNGCompression.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>

std::vector<uint8_t> PNGCompression::compress(const std::vector<uint8_t>& rawData, 
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
        
        // Prepare row pointers (account for bit depth)
        size_t bytesPerPixel = (bitDepth + 7) / 8;
        size_t rowStride = width * channels * bytesPerPixel;
        std::vector<png_bytep> row_pointers(height);
        for (int y = 0; y < height; y++) {
            row_pointers[y] = const_cast<png_bytep>(&rawData[y * rowStride]);
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

std::vector<uint8_t> PNGCompression::decompress(const std::vector<uint8_t>& compressedData) const {
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
