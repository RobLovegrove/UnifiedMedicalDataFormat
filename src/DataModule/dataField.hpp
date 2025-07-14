#ifndef DATAFIELD_HPP
#define DATAFIELD_HPP

#include <string>
#include <vector>
#include <optional>
#include <iostream>

class DataField {
private:
    std::string name;
    std::string type;

public:
    DataField(std::string name, std::string type) 
    : name(name), type(type) {}

    virtual ~DataField() = default;

    std::string getName() const { return name; }
    std::string getType() const { return type; }
    virtual std::size_t getLength() const = 0;

    virtual void encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const = 0;

    // Overload operator<< for Field
    friend std::ostream& operator<<(std::ostream& os, const std::unique_ptr<DataField>& field);
};


class StringField : public DataField {
private:
    size_t length = 0;

public:
    StringField(std::string name, std::string type, size_t length = 0) 
    : DataField(name, type), length(length) {}

    std::size_t getLength() const override { return length; }

    void encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const override;

};

class EnumField : public DataField {
private:
    uint8_t storageSize;  // usually 1 for uint8_t
    std::vector<std::string> enumValues;

    uint8_t lookupEnumValue(const std::string& value) const;

public:
    EnumField(const std::string& name, std::vector<std::string> enumValues, size_t length = 0) 
    : DataField(name, "enum"), storageSize(length), enumValues(enumValues) {}

    size_t getLength() const override { return storageSize; }

    void encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const override;
};

class ObjectField : public DataField {
private:
    std::vector<std::unique_ptr<DataField>> subFields;

public:
    ObjectField(std::string name) : DataField(name, "object") {}

    void encodeToBuffer(const std::string& value, std::vector<uint8_t>& buffer, size_t offset) const override;
    void addSubField(std::unique_ptr<DataField>);

    size_t getLength() const override;
};

#endif