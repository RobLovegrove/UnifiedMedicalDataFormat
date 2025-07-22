#ifndef IMAGEDATA_HPP
#define IMAGEDATA_HPP

#include <nlohmann/json.hpp> 

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Tabular/tabularData.hpp"
#include "../dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"

class ImageData : public TabularData { 

private:

public:
    virtual ~ImageData() override = default;
    explicit ImageData(const std::string& schemaPath, UUID uuid) 
    : TabularData(schemaPath, uuid, ModuleType::Image) {}

};

#endif