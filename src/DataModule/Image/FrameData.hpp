#ifndef FRAMEDATA_HPP
#define FRAMEDATA_HPP

#include "../dataModule.hpp"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <ostream>
#include <istream>

class FrameData : public DataModule {
public:
    std::vector<uint8_t> pixelData;

    FrameData(const std::string& schemaPath, UUID uuid);
    
    // No copy/move operations supported due to DataModule design

    virtual std::streampos writeData(std::ostream& out) const override;
    virtual void decodeData(std::istream& in, size_t actualDataSize) override;
    virtual void printData(std::ostream& out) const override;
};

#endif // FRAMEDATA_HPP