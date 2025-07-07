#ifndef PATIENT_HPP
#define PATIENT_HPP

#include <string>
#include <nlohmann/json.hpp> 
#include <Xref/xref.hpp>

class Patient {
private:
    std::string id;
    std::string familyName;
    std::vector<std::string> givenNames;
    std::string birthDate;
    std::string gender;
    std::string birthSex;

    std::string schema = "http://localhost:8080/schemas/patient/v1.0.json";

    nlohmann::json to_json() const;
    static Patient from_json(const nlohmann::json& j);

public:
    bool writeToFile(std::ofstream& outfile, XRefTable& xref) const;

    void setID(const std::string uuid) {id = uuid; }
    void addGivenName(const std::string name) { givenNames.push_back(name); }
    void setLastName(const std::string name) { familyName = name; }
    void setBirthDate(const std::string date) { birthDate = date; }
    void setGender(const std::string g) { gender = g; }
    void setBirthSex(const std::string bs) { birthSex = bs; }
};

#endif