#ifndef TABULARDATA_HPP
#define TABULARDATA_HPP

#include <nlohmann/json.hpp> 

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"

class TabularData : public DataModule { 

private:
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    StringBuffer stringBuffer;
    size_t rowSize = 0;

    explicit TabularData() {};

    void parseSchema(const nlohmann::json& schemaJson) override;

    std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize) override;

    void decodeRows(std::istream& in, size_t actualDataSize);

public:
    explicit TabularData(const std::string& schemaPath, UUID uuid, ModuleType type);
    virtual ~TabularData() override = default;
    
    void addData(const nlohmann::json& rowData) override;
    void writeBinary(std::ostream& out, XRefTable& xref) override;

    void printRows(std::ostream& out) const;

    static std::unique_ptr<TabularData> fromStream(std::istream& in, uint64_t moduleStartOffset); 


};

#endif