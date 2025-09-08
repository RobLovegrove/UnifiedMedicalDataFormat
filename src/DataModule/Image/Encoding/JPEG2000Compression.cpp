#include "JPEG2000Compression.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

std::vector<uint8_t> JPEG2000Compression::compress(const std::vector<uint8_t>& rawData, 
                                                   int width, int height,
                                                   uint8_t channels, uint8_t bitDepth) const {
    // Validate input data size based on bit depth
    size_t bytesPerPixel = (bitDepth + 7) / 8; // Round up for non-byte-aligned bit depths
    size_t expectedSize = static_cast<size_t>(width) * height * channels * bytesPerPixel;
    if (rawData.size() != expectedSize) {
        std::cerr << "JPEG2000: Input size mismatch. Expected: " << expectedSize 
                  << ", Got: " << rawData.size() << " (bitDepth: " << static_cast<int>(bitDepth) 
                  << ", bytesPerPixel: " << bytesPerPixel << ")" << std::endl;
        return rawData;
    }
    
    std::cerr << "JPEG2000: Starting compression. Size: " << rawData.size() 
              << ", Dimensions: " << width << "x" << height 
              << ", Channels: " << static_cast<int>(channels) 
              << ", BitDepth: " << static_cast<int>(bitDepth) << std::endl;
    
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
            std::cerr << "JPEG2000: Failed to create image" << std::endl;
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
                std::cerr << "JPEG2000: Image component " << c << " data not allocated" << std::endl;
                opj_image_destroy(image);
                return rawData;
            }
        }
        
        // Copy pixel data to image based on bit depth (interleaved for multi-channel)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                
                if (pixel_index >= static_cast<size_t>(width * height)) {
                    continue;
                }
                
                if (channels == 1) {
                    // Single channel (grayscale)
                    size_t data_index = pixel_index * bytesPerPixel;
                    if (data_index + bytesPerPixel <= rawData.size()) {
                        if (bitDepth == 8) {
                            image->comps[0].data[pixel_index] = rawData[data_index];
                        } else if (bitDepth == 16) {
                            // Combine two bytes into 16-bit value (little-endian)
                            uint16_t value = rawData[data_index] | (rawData[data_index + 1] << 8);
                            image->comps[0].data[pixel_index] = value;
                        } else {
                            std::cerr << "JPEG2000: Unsupported bit depth: " << static_cast<int>(bitDepth) << std::endl;
                            return rawData;
                        }
                    }
                } else {
                    // Multi-channel (interleaved) - RGBRGB... format
                    for (int c = 0; c < channels; c++) {
                        size_t data_index = (pixel_index * channels + c) * bytesPerPixel;
                        if (data_index + bytesPerPixel <= rawData.size()) {
                            if (bitDepth == 8) {
                                image->comps[c].data[pixel_index] = rawData[data_index];
                            } else if (bitDepth == 16) {
                                // Combine two bytes into 16-bit value (little-endian)
                                uint16_t value = rawData[data_index] | (rawData[data_index + 1] << 8);
                                image->comps[c].data[pixel_index] = value;
                            } else {
                                std::cerr << "JPEG2000: Unsupported bit depth: " << static_cast<int>(bitDepth) << std::endl;
                                return rawData;
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
            std::cerr << "JPEG2000: Failed to setup encoder" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        // Start compression
        if (!opj_start_compress(codec, image, stream)) {
            std::cerr << "JPEG2000: Failed to start compression" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        if (!opj_encode(codec, stream)) {
            std::cerr << "JPEG2000: Failed to encode" << std::endl;
            opj_stream_destroy(stream);
            opj_image_destroy(image);
            opj_destroy_codec(codec);
            return rawData;
        }
        
        if (!opj_end_compress(codec, stream)) {
            std::cerr << "JPEG2000: Failed to end compression" << std::endl;
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
        
        std::cerr << "JPEG2000: Compression successful. Original: " << rawData.size() 
                  << " bytes, Compressed: " << compressed_data.size() 
                  << " bytes (ratio: " << std::fixed << std::setprecision(2) 
                  << (100.0 * compressed_data.size() / rawData.size()) << "%)" << std::endl;
        
        return compressed_data;
        
    } catch (const std::exception& e) {
        std::cerr << "JPEG2000: Exception during compression: " << e.what() << std::endl;
        return rawData;
    }
}

std::vector<uint8_t> JPEG2000Compression::decompress(const std::vector<uint8_t>& compressedData) const {
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

// OpenJPEG stream functions implementation
OPJ_SIZE_T JPEG2000Compression::memory_read(void* buffer, OPJ_SIZE_T size, void* user_data) {
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

OPJ_SIZE_T JPEG2000Compression::memory_write(void* buffer, OPJ_SIZE_T size, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (ms->current_pos + size <= ms->max_output_size) {
        memcpy(ms->output_buffer + ms->current_pos, buffer, size);
        ms->current_pos += size;
        ms->output_size = std::max(ms->output_size, ms->current_pos);
        return size;
    }
    return 0;
}

OPJ_OFF_T JPEG2000Compression::memory_skip(OPJ_OFF_T offset, void* user_data) {
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

OPJ_BOOL JPEG2000Compression::memory_seek(OPJ_OFF_T offset, void* user_data) {
    MemoryStreamData* ms = (MemoryStreamData*)user_data;
    if (!ms) return OPJ_FALSE;
    if (offset >= 0 && (size_t)offset <= ms->input_size) {
        ms->current_pos = (size_t)offset;
        return OPJ_TRUE;
    }
    return OPJ_FALSE;
}
