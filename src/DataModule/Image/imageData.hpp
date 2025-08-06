#ifndef IMAGEDATA_HPP
#define IMAGEDATA_HPP

#include <nlohmann/json.hpp> 
#include <vector>
#include "FrameData.hpp"
#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Tabular/tabularData.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"

class ImageData : public DataModule { 

private:
    std::vector<std::unique_ptr<FrameData>> frames;

    std::streampos writeData(std::ostream& out) const override;

public:

    virtual ~ImageData() override = default;
    explicit ImageData(const std::string& schemaPath, UUID uuid);
    void addFrame(std::unique_ptr<FrameData> frame) {
        frames.push_back(std::move(frame));
    }
    size_t getNumFrames() const { return frames.size(); }
    virtual void decodeData(std::istream& in, size_t actualDataSize) override;
    virtual void printData(std::ostream& out) const override;
    const std::vector<std::unique_ptr<FrameData>>& getFrames() const { return frames; }
};

#endif