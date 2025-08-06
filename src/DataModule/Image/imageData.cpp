#include "imageData.hpp"
#include "FrameData.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataModule.hpp"

#include "string"
#include <fstream>
#include <iostream>

using namespace std;

ImageData::ImageData(const string& schemaPath, UUID uuid) : DataModule(schemaPath, uuid, ModuleType::Image) {
    std::cout << "ImageData constructor called with schema: " << schemaPath << std::endl;
    initialise();
    std::cout << "ImageData constructor completed successfully" << std::endl;
}

streampos ImageData::writeData(std::ostream& out) const {
    streampos pos = out.tellp();
    size_t totalDataSize = 0;
    for (const auto& frame : frames) {
        // Write each frame's pixel data
        frame->writeData(out);
        totalDataSize += frame->pixelData.size();
    }
    header->setDataSize(totalDataSize);
    return pos;
}

void ImageData::decodeData(std::istream& in, size_t actualDataSize) {
    std::cout << "=== ImageData::decodeData called with dataSize: " << actualDataSize << " ===" << std::endl;
    
    // First, we need to decode the metadata to get the number of frames
    // The metadata should already be decoded by the parent DataModule class
    
    // Get the number of frames from metadata
    size_t numFrames = 1; // Default to 1 frame
    if (!metaDataRows.empty()) {
        std::cout << "Found " << metaDataRows.size() << " metadata rows" << std::endl;
        // Try to find the num_frames field in metadata
        size_t offset = 0;
        for (const auto& field : metaDataFields) {
            if (field->getName() == "num_frames") {
                std::cout << "Found num_frames field at offset " << offset << std::endl;
                // Decode the num_frames value from the first metadata row
                nlohmann::json numFramesJson = field->decodeFromBuffer(metaDataRows[0], offset);
                if (numFramesJson.is_number()) {
                    numFrames = numFramesJson.get<size_t>();
                    std::cout << "Decoded num_frames: " << numFrames << std::endl;
                }
                break;
            }
            offset += field->getLength();
        }
    } else {
        std::cout << "No metadata rows found" << std::endl;
    }
    
    std::cout << "Reading " << numFrames << " frame(s) from ImageData" << std::endl;
    
    // Create frames and read their data
    size_t frameSize = actualDataSize / numFrames;
    for (size_t i = 0; i < numFrames; ++i) {
        // Create a new frame
        auto frame = std::make_unique<FrameData>("./schemas/frame/v1.0.json", UUID());
        
        // Read frame pixel data
        frame->pixelData.resize(frameSize);
        in.read(reinterpret_cast<char*>(frame->pixelData.data()), frameSize);
        if (in.gcount() != static_cast<std::streamsize>(frameSize)) {
            throw std::runtime_error("Failed to read expected frame data size.");
        }
        
        // Add frame to the image
        frames.push_back(std::move(frame));
    }
}

void ImageData::printData(std::ostream& out) const {
    out << "ImageData contains " << frames.size() << " frame(s).\n";
    for (size_t i = 0; i < frames.size(); ++i) {
        out << "Frame " << i << ":\n";
        const auto& frame = frames[i];
        constexpr int width = 16;
        constexpr int height = 16;
        const std::string shades = " .:-=+*#%@";
        if (frame->pixelData.size() < width * height) {
            out << "  Frame data incomplete or incorrect size.\n";
            continue;
        }
        for (int y = 0; y < height; ++y) {
            out << "  ";
            for (int x = 0; x < width; ++x) {
                uint8_t pixel = frame->pixelData[y * width + x];
                char shade = shades[pixel * shades.size() / 256];
                out << shade;
            }
            out << '\n';
        }
    }
}


        