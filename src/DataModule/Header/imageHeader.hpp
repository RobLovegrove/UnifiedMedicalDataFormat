#ifndef IMAGEHEADER_HPP
#define IMAGEHEADER_HPP

#include "dataHeader.hpp"


struct ImageHeader : public DataHeader {
private:
    uint64_t imageOffset = 0;

    std::streampos imageOffsetPos = 0;

    virtual void writeAdditionalOffsets(std::ostream& out) override;
    virtual std::string  outputAdditionalOffsets() const override;

    virtual bool handleExtraField(HeaderFieldType, const std::vector<char>&) override;


public:

    virtual uint64_t getAdditionalOffset() const override { return imageOffset; }
    virtual void seAdditionalOffset(uint64_t offset) override { imageOffset = offset; }
    
    void updateHeader(
        std::ostream& out, uint64_t stringOffset, uint64_t iamgeOffset) override;
};

#endif