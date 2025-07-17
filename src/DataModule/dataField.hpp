#ifndef DATAFIELD_HPP
#define DATAFIELD_HPP

#include "stringBuffer.hpp"

#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp> 


/* =============== DataField =============== */

class DataField {
protected:
    std::string name;
    std::string type;

public:
    DataField(std::string name, std::string type) 
    : name(name), type(type) {}

    virtual ~DataField() = default;

    std::string getName() const { return name; }
    std::string getType() const { return type; }
    virtual std::size_t getLength() const = 0;

    virtual void encodeToBuffer(const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) = 0;
    virtual nlohmann::json decodeFromBuffer(const std::vector<uint8_t>& buffer, size_t offset) const = 0;

    // Overload operator<< for Field
    friend std::ostream& operator<<(std::ostream& os, const std::unique_ptr<DataField>& field);
};


/* =============== StringField =============== */

class StringField : public DataField {
private:
    size_t length = 0;
    //string encoding = "utf-8";

public:
    StringField(std::string name, std::string type, size_t length = 0) 
    : DataField(name, type), length(length) {}

    std::size_t getLength() const override { return length; }

    void encodeToBuffer(const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) override;
    nlohmann::json decodeFromBuffer(const std::vector<uint8_t>& buffer, size_t offset) const override;
    
};

/* =============== VarStringField =============== */

class VarStringField : public DataField {
private:
    uint32_t stringLength = 0;
    uint64_t stringStart;
    StringBuffer* stringBuffer = nullptr;

public:
    VarStringField(std::string name, StringBuffer* stringBuffer) 
    : DataField(name, "varString"), stringBuffer(stringBuffer) {}

    std::size_t getLength() const override { return sizeof(stringStart) + sizeof(stringLength); }

    void encodeToBuffer(const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) override;
    nlohmann::json decodeFromBuffer(const std::vector<uint8_t>& buffer, size_t offset) const override;
    
};

/* =============== EnumField =============== */

class EnumField : public DataField {
private:
    uint8_t storageSize;  // usually 1 for uint8_t
    std::vector<std::string> enumValues;

    uint8_t lookupEnumValue(const std::string& value) const;

public:
    EnumField(const std::string& name, std::vector<std::string> enumValues, size_t length = 0) 
    : DataField(name, "enum"), storageSize(length), enumValues(enumValues) {}

    size_t getLength() const override { return storageSize; }

    void encodeToBuffer(const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) override;
    nlohmann::json decodeFromBuffer(const std::vector<uint8_t>& buffer, size_t offset) const override;
};

/* =============== ObjectField =============== */

class ObjectField : public DataField {
private:
    std::vector<std::unique_ptr<DataField>> subFields;
    size_t length;

public:
    ObjectField(std::string name,
                std::vector<std::unique_ptr<DataField>> subFields, size_t length)
      : DataField(std::move(name), "object"), subFields(std::move(subFields)), length(length) {}

    void encodeToBuffer(
        const nlohmann::json& value, std::vector<uint8_t>& buffer, size_t offset) override;

    nlohmann::json decodeFromBuffer(const std::vector<uint8_t>& buffer, size_t offset) const override;

    void addSubField(std::unique_ptr<DataField>);

    size_t getLength() const override { return length; }
};

#endif