#include "dataField.hpp"

#include <iostream>
#include <memory>

using namespace std;

/* ================== DataField ================== */

ostream& operator<<(ostream& os, const unique_ptr<DataField>& field) {
    os << "Field(name=\"" << field->getName() 
       << "\", type=\"" << field->getType()
       << "\", length=\"" << field->getLength()
       << endl;
    return os;
}

/* ================== StringField ================== */

void StringField::encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const {
    memcpy(&buffer[offset], value.c_str(), std::min(length, value.size()));
}

/* ================== EnumField ================== */

uint8_t EnumField::lookupEnumValue(const std::string& value) const {
    
    auto it = std::find(enumValues.begin(), enumValues.end(), value);
    if (it == enumValues.end()) {
        throw std::invalid_argument("Invalid enum value: " + value);
    }
    return static_cast<uint8_t>(std::distance(enumValues.begin(), it));
}

void EnumField::encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const {
    uint8_t enumValue = lookupEnumValue(value);
    buffer[offset] = enumValue;
}


/* ================== ObjectField ================== */

void ObjectField::encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const {


}

void ObjectField::addSubField(std::unique_ptr<DataField>) {


}

size_t ObjectField::getLength() const {

    size_t totalLength = 0;
    for (const unique_ptr<DataField>& field : subFields) {
        totalLength += field->getLength();
    }
    return totalLength;
}